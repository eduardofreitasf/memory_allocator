/**
 * @file mem_alloc.c
 * @author Eduardo Freitas Fernandes (ef05238@gmail.com)
 * @brief Implementation of a memory allocator
 * @version 1.0.0
 */

 
#include "mem_alloc.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>


/**
 * @brief Size of a memory address
 *
 */
#define PTR_SIZE (sizeof(void *))

/**
 * @brief Size of a header/footer
 * 
 */
#define WORD_SIZE (sizeof(size_t))

/**
 * @brief Minimum size for a memory block
 * 
 */
#define MIN_SIZE (4 * WORD_SIZE)

/**
 * @brief Minimum size for payload
 * 
 */
#define MIN_PAYLOAD (2 * WORD_SIZE)


/**
 * @brief Address of the start of the heap
 * 
 */
static void * heap_start = NULL;

/**
 * @brief Address of the end of the heap
 * 
 */
static void * heap_end = NULL;

/**
 * @brief Address of the head of the free list
 * 
 */
static void * free_list = NULL;



/**
 * @brief Initializes the heap_start and heap_end variables
 * 
 */
static void init_heap(void) {
    heap_start = sbrk(0);
    heap_end = heap_start;
}


/**
 * @brief Adjusts a requested memory block size to meet allocator requirements.
 * 
 * This function ensures the size adheres to three constraints:
 *   1. Meets minimum payload requirements (MIN_PAYLOAD)
 *   2. Includes space for header/footer metadata (2 * WORD_SIZE)
 *   3. Rounds up to maintain 8-byte alignment
 * 
 * @param size Requested payload size in bytes
 * @return Total adjusted block size
 */
static size_t adjust_size(size_t size) {
    if (size < MIN_PAYLOAD) {
        size = MIN_PAYLOAD;
    }
    size_t temp = 7;
    return 2 * WORD_SIZE + ((size + temp) & ~temp);
}



/**
 * @brief Adds a free memory block to the free list
 * 
 * @param ptr address of the memory block
 */
static void free_list_add(void * ptr) {
    unsigned char * temp = (unsigned char *) ptr;
    unsigned char * fl_head = (unsigned char *) free_list;

    // empty free list
    if (free_list == NULL) {
        free_list = ptr;
        // place the next pointer
        memcpy(temp + WORD_SIZE, &ptr, PTR_SIZE);
        // place the previous pointer
        memcpy(temp + 2 * WORD_SIZE, &ptr, PTR_SIZE);
    } else {
        // update the next pointer on the new block
        memcpy(temp + WORD_SIZE, &free_list, PTR_SIZE);
        // update the previous pointer on the new block
        memcpy(temp + 2 * WORD_SIZE, fl_head + 2 * WORD_SIZE, PTR_SIZE);

        // get the last element on the list
        unsigned char * last = NULL;
        memcpy(&last, fl_head + 2 * WORD_SIZE, PTR_SIZE);

        // update the next pointer on the last element
        memcpy(last + WORD_SIZE, &ptr, PTR_SIZE);

        // update the previous pointer on the list head
        memcpy(fl_head + 2 * WORD_SIZE, &ptr, PTR_SIZE);

        // update the head of the list
        free_list = ptr;
    }

}


/**
 * @brief Removes a free memory block from the free list
 * 
 * @param ptr address of the memory block
 */
static void free_list_remove(void * ptr) {
    unsigned char * temp = (unsigned char *) ptr;

    unsigned char * next = NULL, * previous = NULL;
    // get the next block
    memcpy(&next, temp + WORD_SIZE, PTR_SIZE);
    // get the previous block
    memcpy(&previous, temp + 2 * WORD_SIZE, PTR_SIZE);

    // list has only one element
    if (next == ptr) {
        free_list = NULL;
    } else {
        // ptr is the head of the list
        if (free_list == ptr) {
            free_list = next;
        }

        // update the next pointer on the previous block
        memcpy(previous + WORD_SIZE, &next, PTR_SIZE);
        // update the previous pointer on the next block
        memcpy(next + 2 * WORD_SIZE, &previous, PTR_SIZE);
    }

}


/**
 * @brief Finds a free memory block using a Best Fit algorithm
 * 
 * @param size size of the memory block
 * @return address of a memory block
 * @retval NULL if no free block satisfies the size
 */
static void * find_free_block(size_t size) {
    if (free_list == NULL) {
        return NULL;
    }

    unsigned char * iterator = free_list;
    void * start = free_list;
    void * best_fit = NULL;

    do {
        // block has a valid size
        if (*(size_t *)iterator >= size) {
            // found a better block
            if (best_fit == NULL || *(size_t *)iterator < *(size_t *)best_fit) {
                best_fit = iterator;
            }
        }
    
        // go to the next free block
        memcpy(&iterator, iterator + WORD_SIZE, sizeof(void *));
    } while (iterator != start);

    return best_fit;
}



/**
 * @brief Coalesces (merges) the current block with its right neighbor if free.
 * 
 * This function checks if the immediately adjacent block to the right is free, 
 * and if so, merges both blocks into a single larger block. Updates all metadata
 * (header/footer sizes) and maintains the free list consistency.
 *
 * @param ptr Pointer to the header of the current block to coalesce.
 * @param[out] coalesce set to 1 if coalescing occurred, unchanged otherwise.
 * @return Returns the original block pointer (now potentially larger if coalesced).
 *
 * @note Assumes the current block is not at the end of the heap.
 */
static void * coalesce_right(void * ptr, char * coalesce) {
    unsigned char * temp = (unsigned char *) ptr;

    // get the block on the right
    unsigned char * next = temp + *(size_t *) ptr;
    size_t next_size = *(size_t *) next;

    // check if the right is free
    if ((next_size & 1) == 0) {

        *coalesce = 1;

        // update the size of the block
        next_size += *(size_t *)ptr;
        // update the header
        memcpy(ptr, &next_size, WORD_SIZE);
        // update the footer
        size_t offset = 1;
        memcpy(temp + (next_size & ~offset) - WORD_SIZE, &next_size, WORD_SIZE);

        // remove the next block from the free list
        free_list_remove(next);

        // add the block to the free list
        free_list_add(ptr);
    }

    return ptr;
}


/**
 * @brief Coalesces (merges) the current block with its left neighbor if free.
 * 
 * Checks if the immediately adjacent block to the left is free, and if so,
 * merges both blocks into a single larger block. Updates all metadata including
 * header and footer sizes. The merged block will use the left block's address.
 *
 * @param ptr Pointer to the header of the current block to coalesce.
 * @param[out] coalesce set to 1 if coalescing occurred, unchanged otherwise.
 * @return Returns pointer to the merged block (either original or left neighbor).
 *
 * @note Assumes the current block is not at the start of the heap.
 */
static void * coalesce_left(void * ptr, char * coalesce) {
    unsigned char * temp = (unsigned char *) ptr;

    // get the block on the left
    size_t prev_size = *(size_t *)(temp - WORD_SIZE);
    unsigned char * previous = temp - prev_size;

    // check if the left is free
    if ((prev_size & 1) == 0) {

        *coalesce = 1;

        // update the size of the block
        prev_size += *(size_t *) ptr;
        // update the header
        memcpy(previous, &prev_size, WORD_SIZE);
        // update the footer
        size_t offset = 1;
        memcpy(previous + (prev_size & ~offset) - WORD_SIZE, &prev_size, WORD_SIZE);

        ptr = previous;
    }

    return ptr;
}



void * mem_alloc(size_t size) {

    static char init = 0;
    // first call to mem_alloc()
    if (init == 0) {
        // start the heap
        init_heap();
        init = 1;
    }

    if (size == 0) {
        return NULL;
    }

    // adjust the size of the block
    size = adjust_size(size);

    // size cannot be higher than PTRDIFF_MAX
    if (size > PTRDIFF_MAX) {
        return NULL;
    }

    
    // consider RLIMIT_DATA, but first check if it's equal to RLIMIT_INFINITY
    

    size_t remain_size = 0;
    unsigned char * free_block = find_free_block(size);
    // there are no free blocks with size bytes
    if (free_block == NULL) {

        // increment the program break
        free_block = sbrk((intptr_t) size);

        if (free_block == (void *) -1) {

            // sbrk() failed
            errno = ENOMEM;

            return NULL;
        }
        
        // update the end of the heap
        heap_end = sbrk(0);

    } else {
        // remove the free block from the free list
        free_list_remove(free_block);
        // get the remaining size of the block
        remain_size = *(size_t *)free_block - size;

        // create a block with the remaining space
        if (remain_size >= MIN_SIZE) {
            unsigned char * remain = free_block + size;
            // place the header on the remaining block
            memcpy(remain, &remain_size, WORD_SIZE);
            // update the footer on the remaining block
            memcpy(remain + remain_size - WORD_SIZE, &remain_size, WORD_SIZE);

            // add the remaining block to the free list
            free_list_add(remain);
        } else {
            // update the size
            size = *(size_t *)free_block;
        }
    }

    // set the block as allocated
    size |= 1;
    // update the header of the block
    memcpy(free_block, &size, WORD_SIZE);
    // update the footer on the free block
    size_t offset = 1;
    memcpy(free_block + (size & ~offset) - WORD_SIZE, &size, WORD_SIZE);

    return free_block + WORD_SIZE;
}


void mem_free(void * ptr) {

    // invalid pointer, does not belong to the heap
    if (ptr <= heap_start || ptr >= heap_end) {

        printf("mem_free(): invalid pointer\n");
        raise(SIGABRT);

        return;
    }

    unsigned char * temp = (unsigned char *) ptr;
    // get the header of the block
    temp = temp - WORD_SIZE;
    size_t size = *(size_t *)temp;

    // block is free, double free
    if ((size & 1) == 0) {

        printf("mem_free(): double free detected\n");
        raise(SIGABRT);

        return;
    }

    // set the allocation bit to zero
    size_t offset = 1;
    size &= ~offset;
    // change the header
    memcpy(temp, &size, WORD_SIZE);
    // change the footer
    memcpy(temp + size - WORD_SIZE, &size, WORD_SIZE);

    char coalesce = 0;

    if (temp + size < (unsigned char *) heap_end) {
        temp = coalesce_right(temp, &coalesce);
        size = *(size_t *) temp;
    }

    if (temp > (unsigned char *) heap_start) {
        if (coalesce == 1) {
            free_list_remove(temp);
        }

        temp = coalesce_left(temp, &coalesce);
        size = *(size_t *) temp;
    }

    // did not coalesce
    if (coalesce == 0) {
        // add the block to the free list
        free_list_add(temp);
    }

    // last block on the heap
    if (temp + size == heap_end) {
        // remove block from free list
        free_list_remove(temp);
        // decrement the program break
        sbrk(- (intptr_t) size);

        heap_end = sbrk(0);
    }

}


void * mem_resize(void * ptr, size_t size) {

    if (size == 0 && ptr != NULL) {
        mem_free(ptr);
        return NULL;
    }

    // allocate new block
    void * block = mem_alloc(size);
    if (block != NULL && ptr != NULL) {
        // copy the contents to the new block
        memcpy(block, ptr, size);
        // free the old one
        mem_free(ptr);
    }

    return block;
}


void * mem_alloc_clear(size_t n, size_t size) {

    // allocate a new block
    void * block = mem_alloc(n * size);
    if (block != NULL) {
        // set the memory to zero
        memset(block, 0, n * size);
    }

    return block;
}



/**
 * @brief Shows the information of a memory block
 * 
 * @param ptr address of the memory block
 */
static void show_block(void *ptr) {
    unsigned char * temp = (unsigned char *) ptr;
    size_t header = *(size_t *)ptr;
    size_t offset = 1;
    size_t block_size = header & ~offset;
    size_t is_allocated = header & offset;
    
    printf("\n========= Memory Block =========\n");
    printf("| Address    : %p\n", ptr);
    printf("| Status     : %s\n", is_allocated != 0 ? "Allocated" : "Free");
    printf("| Block Size : %zu bytes\n", block_size);
    printf("| Header     : %zu\n", header);

    if (!is_allocated) {
        unsigned char *next = NULL;
        unsigned char *prev = NULL;

        memcpy(&next, temp + WORD_SIZE, PTR_SIZE);
        memcpy(&prev, temp + 2 * WORD_SIZE, PTR_SIZE);

        printf("| Next Free  : %p\n", next);
        printf("| Prev Free  : %p\n", prev);
    } else {
        printf("| Payload    : (in use)\n");
    }

    size_t footer = *(size_t *)(temp + block_size - WORD_SIZE);
    printf("| Footer     : %zu\n", footer);
    printf("================================\n");
}


/**
 * @brief Shows the content of the free list
 * 
 */
static void show_free_list(void) {
    if (free_list == NULL) {
        return;
    }

    printf("\n============ FREE LIST ============\n");

    unsigned char * iterator = free_list;
    do {
        show_block(iterator);
        memcpy(&iterator, iterator + WORD_SIZE, PTR_SIZE);
    } while (iterator != free_list);    
}


void show_heap(void) {
    if (heap_start == NULL) {
        printf("HEAP is NULL\n");
        return;
    }

    printf("\n============= HEAP =============\n");
    
    printf("| START : %p\n", heap_start);
    printf("| END   : %p\n", heap_end);
    printf("| SIZE  : %ld bytes\n", (size_t) ((unsigned char *) heap_end - (unsigned char *) heap_start));
    
    printf("================================\n");

    unsigned char * temp = heap_start;
    size_t offset = 0, dist = 1;
    
    while (temp != heap_end) {
        offset = *(size_t *)temp;
        show_block(temp);
        temp += (offset & ~dist);
    }

    show_free_list();

}

