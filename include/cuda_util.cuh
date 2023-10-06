#ifndef CUDA_UTIL_CUH
#define CUDA_UTIL_CUH

#include <cuda.h>
#include <cuda_runtime.h>
#include <cmath> 

__host__ __device__ inline int __RC2IDX(int row, int col, int columns_per_row) {
    return (row * columns_per_row) + col; 
}

__device__ inline double __pdf(float x, float mean, float sigma) {
    return expf(-0.5 * powf((x - mean) / sigma, 2)) / (
        sigma * sqrtf(2 * M_PI));
}

#endif 