#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>

#include "matrix.h"

#define UNROLL (8)
#define BLOCK_SIZE (32)
#define THREADS (4)

typedef struct matmut_arg_t {
    uint16_t line_stripe;
    const matrix_t *A;
    const matrix_t *B;
    matrix_t *C;
} matmut_arg_t;

extern void do_block(const double *A, const double *B, double *C, uint32_t n, uint32_t x, uint32_t y);

int 
matrix_init(matrix_t *mat, uint32_t m, uint32_t n) 
{
    mat->unaligned_matrix_ptr = calloc(sizeof(double), m*n);

    if(mat->unaligned_matrix_ptr == NULL) {
        return -1;
    }

    uintptr_t ptr = (uintptr_t)mat->unaligned_matrix_ptr;

    /* If not aligned on 32 bytes */
    if(ptr & 0xFF) {
        const uintptr_t padding = 0xFF - (ptr & 0xFF) + 1;

        mat->unaligned_matrix_ptr = realloc(mat->unaligned_matrix_ptr, sizeof(double)*m*n + padding);
        if(mat->unaligned_matrix_ptr == NULL) {
            return -1;
        }

        mat->matrix = (double*)((uintptr_t)mat->unaligned_matrix_ptr + padding);
    }
    else {
        mat->matrix = mat->unaligned_matrix_ptr;
    }

    mat->m = m;
    mat->n = n;

    return 0;
}

void 
matrix_free(matrix_t *mat) 
{
    free(mat->unaligned_matrix_ptr);
}

int 
matrix_add(const matrix_t *A, const matrix_t *B, matrix_t *C) 
{
    return -1;
}

int matrix_compare(const matrix_t *A, const matrix_t *B, double epsilon)
{
    uint32_t n = A->n;

    for(uint32_t i = 0; i < n; i++) {
        for(uint32_t j = 0; j < n; j++) {
            if(fabs(A->matrix[i*n + j] - B->matrix[i*n + j]) > epsilon) {
                return -1;
            }
        }
    }

    return 0;
}

/*static inline void 
do_block(const double *A, const double *B, double *C, uint32_t n, uint32_t x, uint32_t y) 
{
    // Iterate on y, line by line
    for(uint32_t j = y*BLOCK_SIZE; j < (y+1)*BLOCK_SIZE; j++) {
        for(uint32_t k = 0; k < n; k++) {
            for(uint32_t i = x*BLOCK_SIZE ; i < (x+1)*BLOCK_SIZE; i += UNROLL) {
                const uint32_t jni = j * n + i;
                const uint32_t jnk = j * n + k;
                const uint32_t kni = k * n + i;

                C[jni+0] += A[jnk] * B[kni+0];
                C[jni+1] += A[jnk] * B[kni+1];
                C[jni+2] += A[jnk] * B[kni+2];
                C[jni+3] += A[jnk] * B[kni+3];
                C[jni+4] += A[jnk] * B[kni+4];
                C[jni+5] += A[jnk] * B[kni+5];
                C[jni+6] += A[jnk] * B[kni+6];
                C[jni+7] += A[jnk] * B[kni+7];
            }
        }
    }
}*/

static void *
start_handler(void* args)
{
    matmut_arg_t *matmut_args = (matmut_arg_t*)args;

    uint32_t n = matmut_args->A->n;
    uint32_t start_block_line = matmut_args->line_stripe*(n/BLOCK_SIZE)/THREADS;

    for(uint32_t x = 0; x < n/BLOCK_SIZE; x++) {
        for(uint32_t y = start_block_line; y < start_block_line + (n/BLOCK_SIZE)/THREADS; y++) {
            do_block(matmut_args->A->matrix, matmut_args->B->matrix, matmut_args->C->matrix, n, x, y);
        }
    }

    return NULL;
}

int 
matrix_multiply(const matrix_t *A, const matrix_t *B, matrix_t *C) 
{
    if(A->n != B->m) {
        return -1;
    }

    pthread_t threads[THREADS];
    matmut_arg_t args[THREADS];

    for(uint32_t i = 0; i < THREADS; i++) {
        args[i] = (matmut_arg_t){ i, A, B, C };
        pthread_create(&threads[i], NULL, start_handler, &args[i]);
    }

    for(uint32_t i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

int
matrix_multiply_norm(const matrix_t *A, const matrix_t *B, matrix_t *C)
{
    if(A->n != B->m) {
        return -1;
    }

    uint32_t n = A->n;

    for(uint32_t i = 0; i < n; i++) {
        for(uint32_t j = 0; j < n; j++) {
            for(uint32_t k = 0; k < n; k++) {
                C->matrix[i*n + j] += A->matrix[i*n + k] * B->matrix[k*n + j];
            }
        }
    }

    return 0;
}
