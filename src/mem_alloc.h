/**
 * @file mem_alloc.h
 * @author Eduardo Freitas Fernandes (ef05238@gmail.com)
 * @brief Custom memory allocation interface replicating standard library functions.
 * @version 1.0.0
 * 
 * This header provides a custom implementation of memory management functions similar to
 * malloc, free, realloc, and calloc from the standard library.
 */

#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#define _GNU_SOURCE

/* for size_t */
#include <stddef.h>

/**
 * @brief Allocates a block of memory of the given size.
 * 
 * @param size Size in bytes of the memory block to allocate.
 * @return void* Pointer to the allocated memory block.
 * @retval NULL If allocation failed (out of memory or size was 0).
 * @note The allocated memory is not initialized and may contain garbage values.
 * @note On error, the return valur is NULL and errno is set to ENOMEM
 * @warning The returned pointer must be freed with mem_free() when no longer needed.
 */
void * mem_alloc(size_t size);

/**
 * @brief Frees a previously allocated memory block.
 * 
 * @param ptr Pointer to the memory block to free.
 * @note If ptr is NULL, no operation is performed.
 * @warning Passing an invalid pointer (not allocated by mem_alloc/mem_resize) or
 *          a pointer that was already freed results in error.
 */
void mem_free(void * ptr);

/**
 * @brief Resizes a previously allocated memory block.
 * 
 * @param ptr Pointer to the memory block to resize.
 * @param size New size in bytes for the memory block.
 * @return void* Pointer to the resized memory block.
 * @retval NULL If allocation failed (out of memory), original block remains valid.
 * @note If ptr is NULL, behaves like mem_alloc(size).
 * @note On error, the return valur is NULL and errno is set to ENOMEM
 * @warning The returned pointer may be different from the original ptr.
 * @warning The original pointer becomes invalid after successful resize.
 */
void * mem_resize(void * ptr, size_t size);

/**
 * @brief Allocates and clears a block of memory for an array of elements.
 * 
 * @param n Number of elements to allocate.
 * @param size Size in bytes of each element.
 * @return void* Pointer to the allocated memory block.
 * @retval NULL If allocation failed (out of memory or n/size was 0).
 * @note The allocated memory is initialized to zero.
 * @note On error, the return valur is NULL and errno is set to ENOMEM
 * @warning The returned pointer must be freed with mem_free() when no longer needed.
 */
void * mem_alloc_clear(size_t n, size_t size);

/**
 * @brief Displays heap allocation information for debugging purposes.
 * 
 * Shows the current state of allocated and free memory blocks in the heap.
 */
void show_heap(void);

#endif /* MEM_ALLOC_H */