
#include "mem_alloc.h"
#include <stdio.h>

#define COLUMNS 100
#define ROWS 100

/*
    Tests if the allocation went weel, if the pointer is valid, if the allocations don't overlap
*/

static void show_array(int array[], int N) {
    printf("[");
    for (int i = 0; i < N; i++) {
        printf("%2d", array[i]);
    }
    printf(" ]\n");
}

void show_matrix(int **matrix) {
    for (int i = 0; i < ROWS; i++) {
        show_array(matrix[i], COLUMNS);
    }
}

int main(void) {

    int ** matrix = mem_alloc(ROWS * sizeof(int *));
    if (matrix == NULL)
        printf("mem_alloc() failed\n");

    int i, j;
    for (i = 0; i < ROWS; i++) {
        matrix[i] = mem_alloc(COLUMNS * sizeof(int));
        if (matrix[i] == NULL)
            printf("mem_alloc() failed\n");
        else {
            for (j = 0; j < COLUMNS; j++) {
                matrix[i][j] = j % 10;
            }
        }
    }

    show_matrix(matrix);

    show_heap();

    int ** another = mem_alloc(ROWS * sizeof(int *));
    if (another == NULL)
        printf("mem_alloc() failed\n");

    for (i = 0; i < ROWS; i++) {
        another[i] = mem_alloc(COLUMNS * sizeof(int));
        if (another[i] == NULL)
            printf("mem_alloc() failed\n");
        else {
            for (j = 0; j < COLUMNS; j++) {
                another[i][j] = j % 10;
            }
        }
    }

    show_matrix(another);

    show_heap();

    for (i = 0; i < ROWS; i++) {
        if (matrix[i] != NULL) {
            mem_free(matrix[i]);
        }
    }
    mem_free(matrix);

    show_heap();

    for (i = 0; i < ROWS; i++) {
        if (another[i] != NULL) {
            mem_free(another[i]);
        }
    }
    mem_free(another);

    show_heap();

    return 0;
}