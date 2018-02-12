#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "matrix.h"

#define NSEC_PER_SEC 1000000000
#define MAT_SIZE 1024ul

void time_diff(const struct timespec start,
               const struct timespec end,
               uint64_t *seconds,
               uint64_t *nanoseconds) 
{
    int64_t nanosec_diff = end.tv_nsec - start.tv_nsec;
    *nanoseconds = (nanosec_diff < 0) ? NSEC_PER_SEC + nanosec_diff : nanosec_diff;
    *seconds = (end.tv_sec - start.tv_sec) - ((nanosec_diff < 0) ? 1 : 0);
}

int main(int argc, char *argv[]) 
{
    srand(time(NULL));
    
    /* TODO: parse arguments */

    matrix_t A;
    matrix_t B;
    matrix_t C;

    // TODO : check return values
    matrix_init(&A, MAT_SIZE, MAT_SIZE);
    matrix_init(&B, MAT_SIZE, MAT_SIZE);
    matrix_init(&C, MAT_SIZE, MAT_SIZE);

    printf("Initializing matrices... ");
    for(uint32_t i = 0; i < MAT_SIZE; i++) {
        for(uint32_t j = 0; j< MAT_SIZE; j++) {
            A.matrix[i*MAT_SIZE + j] = (double)rand();
            B.matrix[i*MAT_SIZE + j] = (double)rand();
        }
    }
    printf("[OK]\n");

    /* begin benchmark */
    struct timespec start;
    struct timespec end;
    
    printf("Benchmarking... ");
    fflush(stdout);
    if(clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        fprintf(stderr, "couldn't get clock\n");
        return EXIT_FAILURE;
    }

    matrix_multiply(&A, &B, &C);

    if(clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
        fprintf(stderr, "couldn't get clock\n");
        return EXIT_FAILURE;
    }
    printf("[OK]\n");

    /* Print benchmark results */
    uint64_t seconds;
    uint64_t nanoseconds;
    time_diff(start, end, &seconds, &nanoseconds);

    uint64_t totalOps = 2 * MAT_SIZE * MAT_SIZE * MAT_SIZE;
    double totalSeconds = seconds + (double)nanoseconds/NSEC_PER_SEC;

    printf("\nTime spent : %lu.%09lu seconds\n", seconds, nanoseconds);
    printf("Gigaflops : %lf\n", (totalOps/totalSeconds)/1000000000);

    /* Checking results */
    matrix_t C_norm;
    matrix_init(&C_norm, MAT_SIZE, MAT_SIZE);

    matrix_multiply_norm(&A, &B, &C_norm);

    if(matrix_compare(&C, &C_norm, 0.000000001)) {
        printf("\nMatrices are different !\n");
    }

    return EXIT_SUCCESS;
}
