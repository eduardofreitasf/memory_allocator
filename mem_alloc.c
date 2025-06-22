
#include "mem_alloc.h"

#include <unistd.h>
#include <string.h>

/**
 * Allocation bit:
 * - 1 == allocated
 * - 0 == free
 */

/**
 * @brief Number of metadata words stored in a memory block
 * 
 */
#define NR_WORDS 4

/**
 * @brief Minimum size of memory block
 * 
 */
#define MIN_SIZE (NR_WORDS * sizeof(size_t))


/**
 * @brief Holds the memory adress of the start of the heap
 * 
 * Constant value at all times
 * 
 */
static void * heap_start = NULL;

/**
 * @brief Holds the memory adress of the end of the heap
 * 
 * Updated every time sbrk() is used
 * 
 */
static void * heap_end = NULL;

/**
 * @brief Holds the memory adress of the first free block
 */
static void * free_list = NULL;


/**
 * @brief Initializes the heap start
 * 
 */
static void init_heap_start(void) {
    heap_start = sbrk(0);
    heap_end = heap_start;
}


/**
 * @brief Removes the block pointed by ptr from the free list
 * 
 * @param ptr pointer to a block of memory
 */
static void fl_remove(void * ptr) {
    void * temp = ptr + sizeof(size_t);
    // update the previous pointer in the next block
    memcpy(&temp + 2 * sizeof(size_t), ptr + 2 * sizeof(size_t), sizeof(void *));

    temp = ptr + 2 * sizeof(size_t);
    // update the next pointer in the previous block
    memcpy(&temp + sizeof(size_t), ptr + sizeof(size_t), sizeof(void *));
}


/**
 * @brief Adds the block pointed by ptr to the free list
 * 
 * @param ptr pointer to a block of memory
 */
static void fl_add(void * ptr);


/**
 * @brief Joins the memory block pointed by ptr to it's neighbours
 * 
 * @param ptr pointer to a block of memory
 */
static void coalesce(void * ptr);


/**
 * @brief Iterates over the free list to find a free block with size bytes
 * 
 * This search uses a Best Fit algorithm to find a free block,
 * so the whole list is searched.
 * 
 * @param size number of bytes for the block
 * @return memory adress of a block with at least size byes
 * @retval NULL if there isn't a block with size bytes
 */
static void * find_free_block(size_t size) {
    // free list is empty
    if (free_list == NULL) {
        return NULL;
    }

    void * start = free_list;
    void * best_fit = NULL;
    size_t block_size = 0, best_size = 0;

    /*
      the block_size and best_size variables are not necessary,
      but I use them for better readability. 
      I could just do: *(size_t *)start
     */

    // iterate over the free list
    do {
        // read the block size
        memcpy(&block_size, start, sizeof(block_size));

        // check if the block size is enough
        if (block_size >= size) {
            // best_fit == NULL -> first time finding a block with valid size
            // block_size < best_size -> found a block with a better match then the previous best fit
            if (best_fit == NULL || block_size < best_size) {
                best_fit = start;
                best_size = block_size;
            }
        }

        // place the next pointer in start
        memcpy(start, start + sizeof(size_t), sizeof(void *));

    } while (start != free_list);

    // found a valid memory block
    if (best_fit != NULL) {
        // a smaller block will be formed
        if (best_size > size && (best_size - size) >= MIN_SIZE) {
            // remaining piece of the block
            size_t remaining = best_size - size;

            // change the value of the block size in the footer
            memcpy(best_fit + best_size - sizeof(best_size), &remaining, sizeof(remaining));
        
            // change the value of the block size in the header
            memcpy(best_fit + size, &remaining, sizeof(remaining));


            /* update the next and previous pointers in all blocks */

            // new block address
            void * new_block = best_fit + best_size;

            // place the next pointer in the new block
            memcpy(new_block + sizeof(size_t), best_fit + sizeof(size_t), sizeof(void *));

            // update the previous pointer in the next free list element
            void * next = best_fit + sizeof(size_t);
            memcpy(&next + 2 * sizeof(size_t), &new_block, sizeof(void *));

            // update the next pointer in the best fit
            memcpy(best_fit + sizeof(size_t), &new_block, sizeof(void *));
        }


        /* remove the block from the free list */
        fl_remove(best_fit);
    }

    return best_fit;
}


/**
 * @brief Determines the size for a valid block, with atleast size bytes
 * 
 * Minimum size for a block is MIN_SIZE
 * 
 * @param size number of bytes requested by the user
 * @return valid block size in bytes
 */
static size_t adjust_size(size_t size) {
    if (size < MIN_SIZE) {
        return MIN_SIZE;
    }

    return (size + (2 * sizeof(size_t)) + 7) & ~7;
}


void * mem_alloc(size_t size) {
    static char first = 0;
    // start the heap, on the first call to mem_alloc()
    if (first == 0) {
        init_heap_start();
        first = 1;
    }
    
    if (size == 0) {
        return NULL;
    }

    // get a valid block size
    size = adjust_size(size);

    // find a free block
    void * free_block = find_free_block(size);

    // use sbrk() to increment the program break
    if (free_block == NULL) {
        free_block = sbrk(size);

        // update the heap end
        heap_end = sbrk(0);
    }

    // set the allocation bit to 1
    size += 1;

    // place the block size in the header
    memcpy(free_block, &size, sizeof(size));
    
    // place the block size and allocation bit in the footer
    // copy from the header
    memcpy(free_block + size - 1 - sizeof(size), free_block, sizeof(size));

    // return the address of the payload
    return free_block + sizeof(size);
}


void mem_free(void * ptr) {
    // check if the heap is initiated
    if (heap_start == NULL) {
        
        /* SEND SOME SIGNAL (HEAP IS OFF) */

        return;
    }

    // ptr is not a heap memory adress
    if (ptr <= heap_start || ptr >= heap_end) {

        /* SEND SOME SIGNAL (INVALID POINTER) */

        return;
    }

    // pointer to the payload now points to the header
    ptr = ptr - sizeof(size_t);
    // get the header
    size_t * header = (size_t *) ptr;
    
    /* check if the memory block is allocated */
    if ((*header & 1) == 0) {

        /* SEND SOME SIGNAL (DOUBLE FREE) */

        return;
    }

    // set the allocation flag to 0 in the header
    *header = *header & ~1;

    size_t block_size = *header;
    size_t * footer = (size_t *) (ptr + block_size - sizeof(block_size));

    // set the allocation flag to 0 in the footer
    *footer = *footer & ~1;

    
    /* coalescing mechanism */

    size_t * neighbour = NULL;
    // checks if coalescing was necessary
    char coalesce = 0;

    /* check the next block to coalesce */
    if (ptr + block_size < heap_end) {
        neighbour = (size_t *) (ptr + block_size);
        // next block is free
        if ((*neighbour & 1) == 0) {
            coalesce = 1;

            block_size += *neighbour;
            // change the block size in the header
            *header = block_size;
            
            // change the footer
            footer = ((void *) footer) + *neighbour;
            // change the block size in the footer
            *footer = block_size;


            /* FIX THE NEXT AND PREVIOUS POINTERS */


        }
    }
    
    /* check the previous block to coalesce */
    if (ptr - sizeof(block_size) >= heap_start) {
        neighbour = (size_t *) ptr - sizeof(block_size);
        // previous block is free
        if ((*neighbour & 1) == 0) {
            coalesce = 1;

            block_size += *neighbour;

            // change the block size in the footer
            *footer = block_size;

            // change the header
            header = ((void *) header) - *neighbour;
            // change the block size in the header
            *header = block_size;
        }
    }


    /* add the block to the free list, only if it did NOT coalesce */

    // free list empty, create circular list
    if (free_list == NULL) {
        free_list = (void *) header;

        /* SET THE NEXT AND PREVIOUS POINTERS */

    } else if (coalesce != 0) {
        // add new memory block to the free list


    }

}


void * mem_resize(void * prt, size_t size);

