#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#define _GNU_SOURCE

/* for size_t */
#include <stddef.h>

void * mem_alloc(size_t size);

void mem_free(void * ptr);

void * mem_resize(void * ptr, size_t size);

void * mem_alloc_clear(size_t n, size_t size);

/* shows the allocated and free blocks */
/* used only for debugging purposes */
void show_heap(void);


/*
    WARNING:
    FIND A WAY OF AVOIDING THIS:
        mem_free(ptr + 1);
*/


#endif /* MEM_ALLOC_H */