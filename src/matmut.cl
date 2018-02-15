__kernel void matmut(
  __global double *inputA,
  __global double *inputB,
  __global double *outputC,
  int widthA,
  int widthB,
  int heightA,
  int heightB) {

    int row = get_global_id(0);
    int col = get_global_id(1);

    double sum = 0.0;

    for(int i = 0; i < widthA; i++) {
      sum += inputA[row * widthA + i] * inputB[i*widthB + col];
    }

    outputC[row * widthB + col] = sum;
  }
