
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <chrono>

#ifndef CHECK_CUDA
#define CHECK_CUDA(call) do { \
  cudaError_t _e = (call); \
  if (_e != cudaSuccess) { \
    fprintf(stderr, "CUDA error %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(_e)); \
    std::exit(1); \
  } \
} while(0)
#endif

__global__ void axpy_kernel(const double a, const double* __restrict__ x,
                            const double* __restrict__ y, double* __restrict__ d,
                            size_t n) {
  size_t i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n) d[i] = a * x[i] + y[i];
}

int main(int argc, char** argv) {
  // ---- params
  size_t N = (argc > 1) ? std::strtoull(argv[1], nullptr, 10) : (size_t)20'000'000;
  double a = (argc > 2) ? std::atof(argv[2]) : 2.0;
  printf("N=%zu  a=%.3f\n", N, a);

  // ---- host buffers
  std::vector<double> hx(N), hy(N), hd_cpu(N), hd_gpu(N);
  // deterministic init
  unsigned long long seed = 42;
  auto next_val = [&](){
    seed = seed * 2862933555777941757ULL + 3037000493ULL;
    double r = (double)((seed >> 11) & ((1ULL<<53)-1)) / (double)(1ULL<<53); // [0,1)
    return 2.0*r - 1.0; // [-1,1)
  };
  for (size_t i=0;i<N;++i){ hx[i]=next_val(); hy[i]=next_val(); }

  // ---- CPU baseline + timing
  auto t0 = std::chrono::high_resolution_clock::now();
  for (size_t i=0;i<N;++i) hd_cpu[i] = a*hx[i] + hy[i];
  auto t1 = std::chrono::high_resolution_clock::now();
  double cpu_ms = std::chrono::duration<double, std::milli>(t1-t0).count();

  // ---- GPU alloc + H2D
  double *dx=nullptr, *dy=nullptr, *dd=nullptr;
  CHECK_CUDA(cudaMalloc((void**)&dx, N*sizeof(double)));
  CHECK_CUDA(cudaMalloc((void**)&dy, N*sizeof(double)));
  CHECK_CUDA(cudaMalloc((void**)&dd, N*sizeof(double)));
  CHECK_CUDA(cudaMemcpy(dx, hx.data(), N*sizeof(double), cudaMemcpyHostToDevice));
  CHECK_CUDA(cudaMemcpy(dy, hy.data(), N*sizeof(double), cudaMemcpyHostToDevice));

  // ---- Launch
  int block = 256;
  int grid  = (int)((N + block - 1) / block);

  // Use CUDA events for accurate GPU timing
  cudaEvent_t e0, e1;
  CHECK_CUDA(cudaEventCreate(&e0));
  CHECK_CUDA(cudaEventCreate(&e1));

  CHECK_CUDA(cudaEventRecord(e0));
  axpy_kernel<<<grid, block>>>(a, dx, dy, dd, N);
  CHECK_CUDA(cudaEventRecord(e1));
  CHECK_CUDA(cudaEventSynchronize(e1));

  float gpu_ms = 0.0f;
  CHECK_CUDA(cudaEventElapsedTime(&gpu_ms, e0, e1));

  // ---- D2H
  CHECK_CUDA(cudaMemcpy(hd_gpu.data(), dd, N*sizeof(double), cudaMemcpyDeviceToHost));

  // ---- correctness check
  double max_abs_diff = 0.0, sum_cpu = 0.0, sum_gpu = 0.0;
  for (size_t i=0;i<N;++i){
    double diff = std::fabs(hd_cpu[i] - hd_gpu[i]);
    if (diff > max_abs_diff) max_abs_diff = diff;
    sum_cpu += hd_cpu[i];
    sum_gpu += hd_gpu[i];
  }
  bool ok = (max_abs_diff <= 1e-12 * (1.0 + std::fabs(sum_cpu)));
  printf("[CHECK] max|cpu-gpu|=%.3e  sums cpu=%.15f gpu=%.15f => %s\n",
         max_abs_diff, sum_cpu, sum_gpu, ok ? "MATCH" : "MISMATCH");

  // ---- timings
  // Bytes moved: H2D (x,y) + D2H (d) = 24 bytes/elem, plus GPU read/write = bandwidth bound.
  double bytes_moved = (double)N * 24.0;
  double gb = bytes_moved / (1024.0*1024.0*1024.0);
  printf("[TIME] CPU: %.3f ms | GPU kernel: %.3f ms (excl. H2D/D2H)\n", cpu_ms, gpu_ms);
  printf("[BW approx] kernel-only ~ %.2f GB/s for host-visible moves (rough)\n", gb / (gpu_ms/1000.0));

  // ---- cleanup
  cudaEventDestroy(e0); cudaEventDestroy(e1);
  cudaFree(dd); cudaFree(dy); cudaFree(dx);
  return ok ? 0 : 1;
}

\\ same issues like mpi version, i can not install some libraries. so i will try it on colab. 