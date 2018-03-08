#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#include "io.h"
#include "matrix.h"

int read_matrix(FILE *fp, matrix_t *mat)
{
  uint32_t m, n;
  int ret;

  /* First, determine matrix dimensions on first line */
  rewind(fp);

  ret = fscanf(fp, "%"PRIu32" %"PRIu32"\n", &m, &n);
  if(ret < 2) {
    perror("Error while reading matrix dimensions");
    return -1;
  }

  ret = matrix_init(mat, m, n);
  if(ret < 0) {
    perror("Error while Initializing a new matrix");
    return -1;
  }

  /* Then read each matrix element one by one */
  for(size_t y = 0; y < n; y++) {
    for(size_t x = 0; x < n; x++) {
      ret = fscanf(fp, "%f*[ ]", &(mat->matrix[y*n + x]));

      if(ret < 1) {
        perror("Error while reading matrix element");
        return -1;
      }
    }

    ret = fscanf(fp, "\n");
    if(ret < 0) {
      perror("Error while reading line return");
      return -1;
    }
  }

  return 0;
}

int write_matrix(FILE *fp, const matrix_t *mat)
{
  int ret;
  uint32_t m, n;

  m = mat->m;
  n = mat->n;

  /* First write matrix dimensions */
  rewind(fp);

  ret = fprintf(fp, "%"PRIu32" %"PRIu32"\n", m, n);
  if(ret < 0) {
    perror("Error while writing matrix dimensions");
    return -1;
  }

  /* Then write each matrix element one by one */
  for(size_t y = 0; y < mat->n; y++) {
    for(size_t x = 0; x < mat->m - 1; x++) {
      ret = fprintf(fp, "%f ", mat->matrix[y * n + x]);
      if(ret < 0) {
        perror("Error while writing matrix element");
        return -1;
      }
    }

    ret = fprintf(fp, "%f\n", mat->matrix[y * n + m - 1]);
    if(ret < 0) {
      perror("Error while writing last element in line");
      return -1;
    }
  }

  return 0;
}
