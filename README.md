# Memory Allocator

A Memory Allocator implemented in pure C. This project is a reimplementation of `malloc()`, `free()`, `realloc()` and `calloc()` functions, designed to deepen understanding of how dynamic memory management works under the hood.

![Memory Layout Diagram](imgs/mem_alloc-diagram.png)

> This project does **not** use any system libraries like `malloc()` or `free()` internally. All memory management is handled **manually**.

---

## Description

This memory allocator dynamically expands the heap using `sbrk()`, requesting memory from the Operating System as needed. It manages memory by **splitting** and **coalescing** blocks to fulfill allocation requests efficiently. It uses a block-based system with headers and footers to track the size and allocation status of memory regions.

The allocator supports:

- Dynamic allocation with **block metadata**
- Memory **coalescing** (merging of adjacent free blocks)
- **Best-fit** allocation strategy to reduce fragmentation

---


## Block Structure

Each block of memory managed by the allocator includes three main components:

- **Header**: Stores the total size of the block, along with status flags.
- **Payload**: The usable memory returned to the user.
- **Footer**: Mirrors the header, used to aid coalescing and backtracking.

![Memory Block Layout Diagram](imgs/mem-block.png)


When a block is free, it has the following structure:

![Free Memory Block Layout Diagram](imgs/free-block.png)


---

## Free Memory

The allocator uses a linked list of free blocks to manage available memory. Every time a block is freed, the allocator attempts to coalesce it with its neighboring blocks if they are also free, reducing fragmentation.


![Free List Diagram](imgs/free-list.png)

---

## Usage

This API supplies the following functions:

- `void * mem_alloc(size_t size)`: allocates size bytes of memory, same as `malloc()`
- `void mem_free(void * ptr)`: frees the allocated block of memory pointed by `ptr`, same as `free()`
- `void * mem_resize(void * ptr, size_t size)`: resizes the block of memory pointed by ptr to size bytes, same as `realloc()`
- `void * mem_alloc_clear(size_t n, size_t size)`: allocates n elements of size bytes and sets them 0, same as `calloc()`

## Future Plans

- Replace the linked list with a **binary search tree** to improve allocation search times.
- Find a way to check if a pointer is an offset of an allocated pointer, `mem_free(ptr + 1)` should give an error.
    - Implement a hash value or checksum
- Use `mmap()` system call to allocate bigger blocks
