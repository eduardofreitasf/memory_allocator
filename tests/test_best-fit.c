
#include "mem_alloc.h"
#include <stdio.h>

int main(void) {

    void *a = mem_alloc(100);
    void *b = mem_alloc(200);
    void *c = mem_alloc(150);
    void *d = mem_alloc(300);
    void *e = mem_alloc(50);
    void *f = mem_alloc(170);

    show_heap();

    printf("\n---------------------------------------------------\n");

    mem_free(a);
    mem_free(c);
    mem_free(e);

    show_heap();
    printf("\n---------------------------------------------------\n");

    a = mem_alloc(60);

    show_heap();
    printf("\n---------------------------------------------------\n");

    return 0;
}