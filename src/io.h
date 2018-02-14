/* IO file operations */

#ifndef __IO_H
#define __IO_H

#include <stdio.h>
#include "matrix.h"

int read_matrix(FILE *fp, matrix_t *mat);
int write_matrix(FILE *fp, const matrix_t *mat);

#endif
