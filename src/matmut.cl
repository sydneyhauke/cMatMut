__kernel void matmut(
  __global float *inputA,
  __global float *inputB,
  __global float *outputC,
  int widthA,
  int widthB,
  int heightA,
  int heightB) {

    int row = get_global_id(0);
    int col = get_global_id(1);

    float sum = 0.0;

    for(int i = 0; i < widthA; i++) {
      sum += inputA[row * widthA + i] * inputB[i*widthB + col];
    }

    outputC[row * widthB + col] = sum;
  }
