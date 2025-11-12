#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct {
    int rows;
    int cols;
    double** data;
} Matrix;

// Matrix creation and destruction
Matrix* matrix_create(int rows, int cols);
void matrix_free(Matrix* m);
Matrix* matrix_copy(Matrix* m);

// Matrix initialization
void matrix_fill(Matrix* m, double value);
void matrix_randomize(Matrix* m, double min, double max);
void matrix_set(Matrix* m, int row, int col, double value);
double matrix_get(Matrix* m, int row, int col);

// Matrix operations
Matrix* matrix_add(Matrix* a, Matrix* b);
Matrix* matrix_subtract(Matrix* a, Matrix* b);
Matrix* matrix_multiply(Matrix* a, Matrix* b);
Matrix* matrix_multiply_scalar(Matrix* m, double scalar);
Matrix* matrix_transpose(Matrix* m);
Matrix* matrix_hadamard(Matrix* a, Matrix* b); // Element-wise multiplication

// Matrix utilities
void matrix_print(Matrix* m);
void matrix_map(Matrix* m, double (*func)(double));

#endif
