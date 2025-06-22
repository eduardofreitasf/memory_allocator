
#include "mem_alloc.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {

    printf("=== Memory Allocator tests ===\n");


    int * x = (int *) mem_alloc(1000 * sizeof(int));
    x[0] = 3;

    printf("x: %p\n", x);
    printf("*x: %d\n", *x);

    mem_free(x);

    return 0;
}