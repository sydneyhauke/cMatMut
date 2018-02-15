#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "matrix.h"
#include "io.h"

#define NSEC_PER_SEC 1000000000

typedef int (*matrix_mult_fct)(const matrix_t *, const matrix_t *, matrix_t *);

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
    int ret;
    FILE *fp1;
    FILE *fp2;
    FILE *fp3;
    matrix_t A;
    matrix_t B;
    matrix_t C;

    matrix_mult_fct matrix_multiply;

    if(argc < 4) {
      fputs("Not enough arguments\n", stderr);
      return EXIT_FAILURE;
    }

    fputs("Begin ", stdout);
    if(strcmp(argv[1], "-m") == 0) {
      /* Set to multi-threaded computation */
      matrix_multiply = matrix_multiply_mt;
      fputs("multi threaded ", stdout);
    }
    else if(strcmp(argv[1], "-o") == 0) {
      /* Set to OpenCL computation */
      matrix_multiply = matrix_multiply_cl;
      fputs("OpenCL ", stdout);
    }
    else {
      /* Set to single threaded computation */
      matrix_multiply = matrix_multiply_st;
      fputs("single threaded ", stdout);
    }
    fputs("computation\n", stdout);

    fp1 = fopen(argv[2], "r");
    if(fp1 == NULL) {
      perror("Error while opening file of matrix A");
      return EXIT_FAILURE;
    }

    fp2 = fopen(argv[3], "r");
    if(fp2 == NULL) {
      perror("Error while opening file of matrix B");
      return EXIT_FAILURE;
    }

    fp3 = fopen("result.txt", "w");
    if(fp3 == NULL) {
      perror("Error while opening result file");
      return EXIT_FAILURE;
    }

    printf("Reading matrices... ");
    fflush(stdout);

    ret = read_matrix(fp1, &A);
    if(ret < 0) {
      return EXIT_FAILURE;
    }

    ret = read_matrix(fp2, &B);
    if(ret < 0) {
      return EXIT_FAILURE;
    }

    matrix_init(&C, A.m, B.n);

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

    /* Print results in a file */
    ret = write_matrix(fp3, &C);
    if(ret < 0) {
      return EXIT_FAILURE;
    }

    /* Print benchmark results */
    uint64_t seconds;
    uint64_t nanoseconds;
    time_diff(start, end, &seconds, &nanoseconds);

    uint64_t totalOps = 2 * A.m * A.n * A.n;
    double totalSeconds = seconds + (double)nanoseconds/NSEC_PER_SEC;

    printf("\nTime spent : %lu.%09lu seconds\n", seconds, nanoseconds);
    printf("Gigaflops : %lf\n", (totalOps/totalSeconds)/1000000000);

    /* Checking results */
    matrix_t C_norm;
    matrix_init(&C_norm, A.m, B.n);

    matrix_multiply_st(&A, &B, &C_norm);

    if(matrix_compare(&C, &C_norm, 0.000000001)) {
        printf("\nMatrices are different !\n");
    }

    return EXIT_SUCCESS;
}
