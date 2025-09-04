// task9_openmp.c
// This program computes d = x + y and compares OpenMP vs serial.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include <string.h>

static inline double now_sec(void) {
    return omp_get_wtime(); // high-res wall clock
}

static inline int approx_equal(double a, double b, double rtol, double atol) {
    double diff = fabs(a - b);
    return diff <= (atol + rtol * fabs(b));
}

int main(int argc, char** argv) {
    // ---- Input size ----
    size_t N = (argc > 1) ? strtoull(argv[1], NULL, 10) : (size_t)5e6; // default: 5 million
    int do_reduction = 1; // set 0 to skip sum(d)

    printf("N = %zu\n", N);

    // ---- Allocate ----
    double *x = (double*) aligned_alloc(64, N * sizeof(double));
    double *y = (double*) aligned_alloc(64, N * sizeof(double));
    double *d_serial = (double*) aligned_alloc(64, N * sizeof(double));
    double *d_omp    = (double*) aligned_alloc(64, N * sizeof(double));
    if(!x || !y || !d_serial || !d_omp) {
        fprintf(stderr, "Allocation failed.\n");
        return 1;
    }

    // ---- Initialize reproducibly ----
    // keep data identical across runs
    unsigned long long seed = 42;
    for (size_t i = 0; i < N; ++i) {
        // simple LCG
        seed = seed * 2862933555777941757ULL + 3037000493ULL;
        double rx = (double)(seed >> 33) / (double)(1ULL<<31); // [0,2)
        x[i] = rx - 1.0; // [-1,1)
        seed = seed * 2862933555777941757ULL + 3037000493ULL;
        double ry = (double)(seed >> 33) / (double)(1ULL<<31);
        y[i] = ry - 1.0;
    }

    // ---- Serial baseline ----
    double t0 = now_sec();
    for (size_t i = 0; i < N; ++i) {
        d_serial[i] = x[i] + y[i];
    }
    double t1 = now_sec();
    double serial_time = t1 - t0;

    // Optional serial reduction
    double serial_sum = 0.0;
    if (do_reduction) {
        for (size_t i = 0; i < N; ++i) serial_sum += d_serial[i];
    }

    // ---- OpenMP parallel ----
    double t2 = now_sec();
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < N; ++i) {
        d_omp[i] = x[i] + y[i];
    }
    double t3 = now_sec();
    double omp_time = t3 - t2;

    // Check correctness elementwise (cheap probe on a few positions + a norm)
    // Compute max abs diff
    double max_abs_diff = 0.0;
    for (size_t i = 0; i < N; ++i) {
        double diff = fabs(d_serial[i] - d_omp[i]);
        if (diff > max_abs_diff) max_abs_diff = diff;
    }
    printf("[CHECK] max |d_serial - d_omp| = %.3e => %s\n",
           max_abs_diff, (max_abs_diff <= 1e-12 ? "OK" : "MISMATCH"));

    // Optional OpenMP reduction on d_omp
    double omp_sum = 0.0;
    if (do_reduction) {
        #pragma omp parallel for reduction(+:omp_sum) schedule(static)
        for (size_t i = 0; i < N; ++i) {
            omp_sum += d_omp[i];
        }
        printf("[CHECK] sum(d) serial=%.15f  openmp=%.15f  => %s\n",
               serial_sum, omp_sum,
               approx_equal(serial_sum, omp_sum, 1e-12, 0.0) ? "MATCH" : "MISMATCH");
    }

    // ---- Timing ----
    int threads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        threads = omp_get_num_threads();
    }
    printf("[TIME] serial: %.6f s | openmp (%d threads): %.6f s | speedup: %.2fx\n",
           serial_time, threads, omp_time,
           (omp_time > 0.0 ? serial_time / omp_time : 0.0));

    free(d_omp);
    free(d_serial);
    free(y);
    free(x);
    return 0;
}

//            Out Put                 //
//gcc -O3 -march=native -ffast-math -fopenmp -std=c11 task9_openmp.c -o task9_openmp -lm
//OMP_NUM_THREADS=1 ./task9_openmp 10000000
//OMP_NUM_THREADS=4 ./task9_openmp 10000000
//N = 10000000
//[CHECK] max |d_serial - d_omp| = 0.000e+00 => OK
//[CHECK] sum(d) serial=-10002052.154661707580090  openmp=-10002052.154661707580090  => MATCH
//[TIME] serial: 0.045380 s | openmp (1 threads): 0.045021 s | speedup: 1.01x
//N = 10000000
//[CHECK] max |d_serial - d_omp| = 0.000e+00 => OK
//[CHECK] sum(d) serial=-10002052.154661707580090  openmp=-10002052.154661707580090  => MATCH
//[TIME] serial: 0.046151 s | openmp (4 threads): 0.014158 s | speedup: 3.26x