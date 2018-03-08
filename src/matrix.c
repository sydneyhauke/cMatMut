#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>
#include <CL/cl.h>

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

extern void do_block(const float *A, const float *B, float *C, uint32_t n, uint32_t x, uint32_t y);

int
matrix_init(matrix_t *mat, uint32_t m, uint32_t n)
{
    mat->unaligned_matrix_ptr = calloc(sizeof(float), m*n);

    if(mat->unaligned_matrix_ptr == NULL) {
        return -1;
    }

    uintptr_t ptr = (uintptr_t)mat->unaligned_matrix_ptr;

    /* If not aligned on 32 bytes */
    if(ptr & 0xFF) {
        const uintptr_t padding = 0xFF - (ptr & 0xFF) + 1;

        mat->unaligned_matrix_ptr = realloc(mat->unaligned_matrix_ptr, sizeof(float)*m*n + padding);
        if(mat->unaligned_matrix_ptr == NULL) {
            return -1;
        }

        mat->matrix = (float*)((uintptr_t)mat->unaligned_matrix_ptr + padding);
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

int matrix_compare(const matrix_t *A, const matrix_t *B, float epsilon)
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
matrix_multiply_mt(const matrix_t *A, const matrix_t *B, matrix_t *C)
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
matrix_multiply_st(const matrix_t *A, const matrix_t *B, matrix_t *C)
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

int
matrix_multiply_cl(const matrix_t *A, const matrix_t *B, matrix_t *C)
{
  cl_int ciErrNum;

  const cl_int hA = A->m;
  const cl_int wA = A->n;
  const cl_int hB = B->m;
  const cl_int wB = B->n;
  const cl_int hC = C->m;
  const cl_int wC = C->n;

  cl_platform_id platform;
  clGetPlatformIDs(1, &platform, NULL);

  cl_device_id device;
  clGetDeviceIDs(
    platform,
    CL_DEVICE_TYPE_ALL,
    1,
    &device,
    NULL
  );

  cl_context_properties cps[3] = {
    CL_CONTEXT_PLATFORM,
    (cl_context_properties)platform,
    0
  };

  cl_context ctx = clCreateContext(
    cps,
    1,
    &device,
    NULL,
    NULL,
    &ciErrNum
  );

  cl_command_queue queue = clCreateCommandQueue(
    ctx,
    device,
    0,
    &ciErrNum
  );

  cl_mem bufferA = clCreateBuffer(
    ctx,
    CL_MEM_READ_ONLY,
    A->m * A->n * sizeof(float),
    NULL,
    &ciErrNum
  );

  clEnqueueWriteBuffer(
    queue,
    bufferA,
    CL_TRUE,
    0,
    A->m * A->n * sizeof(float),
    (void *)A->matrix,
    0,
    NULL,
    NULL
  );

  cl_mem bufferB = clCreateBuffer(
    ctx,
    CL_MEM_READ_ONLY,
    B->m * B->n * sizeof(float),
    NULL,
    &ciErrNum
  );

  clEnqueueWriteBuffer(
    queue,
    bufferB,
    CL_TRUE,
    0,
    B->m * B->n * sizeof(float),
    (void *)B->matrix,
    0,
    NULL,
    NULL
  );

  cl_mem bufferC = clCreateBuffer(
    ctx,
    CL_MEM_WRITE_ONLY,
    A->m * B->n * sizeof(float),
    NULL,
    &ciErrNum
  );

  FILE* kernelFp = fopen(MATMUT_KERNEL_FILE, "r");
  if(kernelFp == NULL) {
    perror("Error while opening CL kernel file");
    return -1;
  }

  if(fseek(kernelFp, 0, SEEK_END) < 0) {
    perror("Error while seeking CL kernel file");
    return -1;
  }

  long length = ftell(kernelFp);

  rewind(kernelFp);

  char *kernelSource = calloc(length, sizeof(char));
  if(kernelSource == NULL) {
    perror("Error while allocating memory for kernel source");
    return -1;
  }

  if(fread((void*)kernelSource, length, sizeof(char), kernelFp) < length) {
    if(ferror(kernelFp)) {
      perror("Error while reading CL kernel file");
      return -1;
    }
  }

  cl_program program = clCreateProgramWithSource(
    ctx,
    1,
    (const char**)&kernelSource,
    NULL,
    &ciErrNum
  );

  ciErrNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

  cl_kernel matmut_kernel = clCreateKernel(
    program,
    "matmut",
    &ciErrNum
  );

  clSetKernelArg(matmut_kernel, 0, sizeof(cl_mem), (void*)&bufferA);
  clSetKernelArg(matmut_kernel, 1, sizeof(cl_mem), (void*)&bufferB);
  clSetKernelArg(matmut_kernel, 2, sizeof(cl_mem), (void*)&bufferC);
  clSetKernelArg(matmut_kernel, 3, sizeof(cl_int), (void*)&wA);
  clSetKernelArg(matmut_kernel, 4, sizeof(cl_int), (void*)&wB);
  clSetKernelArg(matmut_kernel, 5, sizeof(cl_int), (void*)&hA);
  clSetKernelArg(matmut_kernel, 6, sizeof(cl_int), (void*)&hB);

  size_t localws[2] = {2, 2};
  size_t globalws[2] = {wC, hC};

  ciErrNum = clEnqueueNDRangeKernel(
    queue,
    matmut_kernel,
    2,
    NULL,
    globalws,
    localws,
    0,
    NULL,
    NULL
  );

  ciErrNum = clEnqueueReadBuffer(
    queue,
    bufferC,
    CL_TRUE,
    0,
    wC*hC*sizeof(float),
    (void*)C->matrix,
    0,
    NULL,
    NULL
  );

  clReleaseKernel(matmut_kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseMemObject(bufferA);
  clReleaseMemObject(bufferB);
  clReleaseMemObject(bufferC);
  clReleaseContext(ctx);

  free(kernelSource);
  fclose(kernelFp);

  return 0;
}
