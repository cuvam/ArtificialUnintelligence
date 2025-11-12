#include "matrix.h"
#include <time.h>

Matrix* matrix_create(int rows, int cols) {
    Matrix* m = (Matrix*)malloc(sizeof(Matrix));
    m->rows = rows;
    m->cols = cols;
    m->data = (double**)malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        m->data[i] = (double*)calloc(cols, sizeof(double));
    }
    return m;
}

void matrix_free(Matrix* m) {
    if (m == NULL) return;
    for (int i = 0; i < m->rows; i++) {
        free(m->data[i]);
    }
    free(m->data);
    free(m);
}

Matrix* matrix_copy(Matrix* m) {
    Matrix* copy = matrix_create(m->rows, m->cols);
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            copy->data[i][j] = m->data[i][j];
        }
    }
    return copy;
}

void matrix_fill(Matrix* m, double value) {
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            m->data[i][j] = value;
        }
    }
}

void matrix_randomize(Matrix* m, double min, double max) {
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            double random = ((double)rand() / RAND_MAX);
            m->data[i][j] = min + random * (max - min);
        }
    }
}

void matrix_set(Matrix* m, int row, int col, double value) {
    if (row >= 0 && row < m->rows && col >= 0 && col < m->cols) {
        m->data[row][col] = value;
    }
}

double matrix_get(Matrix* m, int row, int col) {
    if (row >= 0 && row < m->rows && col >= 0 && col < m->cols) {
        return m->data[row][col];
    }
    return 0.0;
}

Matrix* matrix_add(Matrix* a, Matrix* b) {
    if (a->rows != b->rows || a->cols != b->cols) {
        fprintf(stderr, "Matrix dimensions must match for addition\n");
        return NULL;
    }

    Matrix* result = matrix_create(a->rows, a->cols);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            result->data[i][j] = a->data[i][j] + b->data[i][j];
        }
    }
    return result;
}

Matrix* matrix_subtract(Matrix* a, Matrix* b) {
    if (a->rows != b->rows || a->cols != b->cols) {
        fprintf(stderr, "Matrix dimensions must match for subtraction\n");
        return NULL;
    }

    Matrix* result = matrix_create(a->rows, a->cols);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            result->data[i][j] = a->data[i][j] - b->data[i][j];
        }
    }
    return result;
}

Matrix* matrix_multiply(Matrix* a, Matrix* b) {
    if (a->cols != b->rows) {
        fprintf(stderr, "Matrix dimensions incompatible for multiplication: (%d,%d) x (%d,%d)\n",
                a->rows, a->cols, b->rows, b->cols);
        return NULL;
    }

    Matrix* result = matrix_create(a->rows, b->cols);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < a->cols; k++) {
                sum += a->data[i][k] * b->data[k][j];
            }
            result->data[i][j] = sum;
        }
    }
    return result;
}

Matrix* matrix_multiply_scalar(Matrix* m, double scalar) {
    Matrix* result = matrix_create(m->rows, m->cols);
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            result->data[i][j] = m->data[i][j] * scalar;
        }
    }
    return result;
}

Matrix* matrix_transpose(Matrix* m) {
    Matrix* result = matrix_create(m->cols, m->rows);
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            result->data[j][i] = m->data[i][j];
        }
    }
    return result;
}

Matrix* matrix_hadamard(Matrix* a, Matrix* b) {
    if (a->rows != b->rows || a->cols != b->cols) {
        fprintf(stderr, "Matrix dimensions must match for Hadamard product\n");
        return NULL;
    }

    Matrix* result = matrix_create(a->rows, a->cols);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            result->data[i][j] = a->data[i][j] * b->data[i][j];
        }
    }
    return result;
}

void matrix_print(Matrix* m) {
    printf("Matrix (%d x %d):\n", m->rows, m->cols);
    for (int i = 0; i < m->rows; i++) {
        printf("[ ");
        for (int j = 0; j < m->cols; j++) {
            printf("%.4f ", m->data[i][j]);
        }
        printf("]\n");
    }
    printf("\n");
}

void matrix_map(Matrix* m, double (*func)(double)) {
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            m->data[i][j] = func(m->data[i][j]);
        }
    }
}
