
#include "mem_alloc.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>


#define PTR_SIZE (sizeof(void *))
#define WORD_SIZE (sizeof(size_t))

#define MIN_SIZE (4 * WORD_SIZE)
#define MIN_PAYLOAD (2 * WORD_SIZE)


static void * heap_start = NULL;
static void * heap_end = NULL;
static void * free_list = NULL;


/**
 * ===========================================
 *                  UTILS
 * ===========================================
 */

static void init_heap(void) {
    heap_start = sbrk(0);
    heap_end = heap_start;
}

static size_t adjust_size(size_t size) {
    if (size < MIN_PAYLOAD) {
        size = MIN_PAYLOAD;
    }
    return 2 * WORD_SIZE + ((size + 7) & ~7);
}



/*
 * ===========================================
 *             EXPLICIT FREE LIST
 * ===========================================
 */

static void free_list_add(void * ptr) {

    // empty free list
    if (free_list == NULL) {
        free_list = ptr;
        // place the next pointer
        memcpy(ptr + WORD_SIZE, &ptr, PTR_SIZE);
        // place the previous pointer
        memcpy(ptr + 2 * WORD_SIZE, &ptr, PTR_SIZE);
    } else {
        // update the next pointer on the new block
        memcpy(ptr + WORD_SIZE, &free_list, PTR_SIZE);
        // update the previous pointer on the new block
        memcpy(ptr + 2 * WORD_SIZE, free_list + 2 * WORD_SIZE, PTR_SIZE);

        // get the last element on the list
        void * last = NULL;
        memcpy(&last, free_list + 2 * WORD_SIZE, PTR_SIZE);

        // update the next pointer on the last element
        memcpy(last + WORD_SIZE, &ptr, PTR_SIZE);

        // update the previous pointer on the list head
        memcpy(free_list + 2 * WORD_SIZE, &ptr, PTR_SIZE);

        // update the head of the list
        free_list = ptr;
    }

}

static void free_list_remove(void * ptr) {

    void * next = NULL, * previous = NULL;
    // get the next block
    memcpy(&next, ptr + WORD_SIZE, PTR_SIZE);
    // get the previous block
    memcpy(&previous, ptr + 2 * WORD_SIZE, PTR_SIZE);

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

static void * find_free_block(size_t size) {
    if (free_list == NULL) {
        return NULL;
    }

    void * iterator = free_list;
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



/*
 * ===========================================
 *                COALESCING
 * ===========================================
 */

/**
 * @brief 
 * 
 * Assumes it is not the end of the heap
 * 
 * @param ptr 
 * @return void* 
 */
static void * coalesce_right(void * ptr, char * coalesce) {

    // get the block on the right
    void * next = ptr + *(size_t *) ptr;
    size_t next_size = *(size_t *) next;

    // check if the right is free
    if ((next_size & 1) == 0) {

        *coalesce = 1;

        // update the size of the block
        next_size += *(size_t *)ptr;
        // update the header
        memcpy(ptr, &next_size, WORD_SIZE);
        // update the footer
        memcpy(ptr + (next_size & ~1) - WORD_SIZE, &next_size, WORD_SIZE);

        // remove the next block from the free list
        free_list_remove(next);

        // add the block to the free list
        free_list_add(ptr);
    }

    return ptr;
}

/**
 * @brief 
 * 
 * Assumes it is not the heap start
 * 
 * @param ptr 
 * @return void* 
 */
static void * coalesce_left(void * ptr, char * coalesce) {

    // get the block on the left
    size_t prev_size = *(size_t *)(ptr - WORD_SIZE);
    void * previous = ptr - prev_size;

    // check if the left is free
    if ((prev_size & 1) == 0) {

        *coalesce = 1;

        // update the size of the block
        prev_size += *(size_t *) ptr;
        // update the header
        memcpy(previous, &prev_size, WORD_SIZE);
        // update the footer
        memcpy(previous + (prev_size & ~1) - WORD_SIZE, &prev_size, WORD_SIZE);

        ptr = previous;
    }

    return ptr;
}



/**
 * ===========================================
 *               API FUNCTIONS
 * ===========================================
 */


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


    // size cannot be higher than PTRDIFF_MAX
    // consider RLIMIT_DATA, but first check if it's equal to RLIMIT_INFINITY
    

    // adjust the size of the block
    size = adjust_size(size);

    size_t remain_size = 0;
    void * free_block = find_free_block(size);
    // there are no free blocks with size bytes
    if (free_block == NULL) {

        // increment the program break
        free_block = sbrk(size);

        if (free_block == (void *) -1) {

            // sbrk() failed

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
            void * remain = free_block + size;
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
    memcpy(free_block + (size & ~1) - WORD_SIZE, &size, WORD_SIZE);

    return free_block + WORD_SIZE;
}


void mem_free(void * ptr) {

    if (ptr <= heap_start || ptr >= heap_end) {

        // signal INVALID POINTER

        return;
    }

    // get the header of the block
    ptr = ptr - WORD_SIZE;
    size_t size = *(size_t *)ptr;

    // block is free
    if ((size & 1) == 0) {

        // signal DOUBLE FREE

        return;
    }

    // set the allocation bit to zero
    size &= ~1;
    // change the header
    memcpy(ptr, &size, WORD_SIZE);
    // change the footer
    memcpy(ptr + size - WORD_SIZE, &size, WORD_SIZE);

    char coalesce = 0;

    if (ptr + size < heap_end) {
        ptr = coalesce_right(ptr, &coalesce);
        size = *(size_t *) ptr;
    }

    if (ptr > heap_start) {
        if (coalesce == 1) {
            free_list_remove(ptr);
        }

        ptr = coalesce_left(ptr, &coalesce);
        size = *(size_t *) ptr;
    }

    // did not coalesce
    if (coalesce == 0) {
        // add the block to the free list
        free_list_add(ptr);
    }

    // last block on the heap
    if (ptr + size == heap_end) {
        // remove block from free list
        free_list_remove(ptr);
        // decrement the program break
        sbrk(-size);

        heap_end = sbrk(0);
    }

}


void * mem_resize(void * ptr, size_t size) {

    void * block = mem_alloc(size);
    if (block != NULL) {
        memcpy(block, ptr, size);
        mem_free(ptr);
    }

    return block;
}


void * mem_alloc_clear(size_t n, size_t size) {

    void * block = mem_alloc(n * size);
    if (block != NULL) {
        memset(block, 0, n * size);
    }

    return block;
}


static void show_block(void * ptr) {

    size_t size = *(size_t *) ptr;

    printf("------------------------------\n");
    printf("Address: %p\n", ptr);
    printf("Status: %s\n", (size & 1) ? "Allocated" : "Free");
    printf("Block size: %ld\n", size & ~1);
    printf("Header: %ld\n", *(size_t *)ptr);
    if ((size & 1) == 0) {
        void * temp = NULL;
        memcpy(&temp, ptr + WORD_SIZE, PTR_SIZE);
        printf("Next: %p\n", temp);
        memcpy(&temp, ptr + 2 * WORD_SIZE, PTR_SIZE);
        printf("Previous: %p\n", temp);
    } else {
        printf("Payload: ...\n");
    }
    printf("Footer: %ld\n", *(size_t *)(ptr + (size & ~1) - WORD_SIZE));
}

static void show_free_list(void) {
    if (free_list == NULL) {
        return;
    }

    printf("\n============ FREE LIST ============\n");

    void * iterator = free_list;
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

    printf("\n============ HEAP ============\n");
    
    printf("START: %p\n", heap_start);
    printf("END: %p\n", heap_end);
    printf("HEAP SIZE: %ld\n", (size_t) (heap_end - heap_start));
    
    void * temp = heap_start;
    size_t offset = 0;
    
    while (temp != heap_end) {
        offset = *(size_t *)temp;
        show_block(temp);
        temp += (offset & ~1);    
    }
    
    printf("==============================\n");

}

