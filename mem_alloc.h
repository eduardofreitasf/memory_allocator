#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#define _GNU_SOURCE

/* for size_t */
#include <stddef.h>

void * mem_alloc(size_t size);

void mem_free(void * ptr);

void * mem_resize(void * prt, size_t size);

#endif /* MEM_ALLOC_H */