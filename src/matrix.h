#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>

/* Matrix of variable height and width */
typedef struct matrix_t matrix_t;

struct matrix_t {
    float *unaligned_matrix_ptr;
    float *matrix;
    uint32_t m;
    uint32_t n;
};

/* Matrix creation/destruction */
int matrix_init(matrix_t *mat, uint32_t m, uint32_t n);
void matrix_free(matrix_t *mat);

/* Matrix operation functions */
int matrix_add(const matrix_t *A, const matrix_t *B, matrix_t *C);
int matrix_multiply_st(const matrix_t *A, const matrix_t *B, matrix_t *C);
int matrix_multiply_mt(const matrix_t *A, const matrix_t *B, matrix_t *C);
int matrix_multiply_cl(const matrix_t *A, const matrix_t *B, matrix_t *C);

int matrix_compare(const matrix_t *A, const matrix_t *B, float epsilon);

/* OpenCL kernel source file */
#define MATMUT_KERNEL_FILE "src/matmut.cl"

#endif
