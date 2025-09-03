// task06_part1.c
// Part 1: build A (1000x1000 Gaussian mean=1, std=1) and compute C = FFT2(A)
// Requires: libfftw3-dev
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <fftw3.h>

#ifndef N
#define N 1000
#endif

// Box–Muller standard normal (mean 0, std 1)
static double randn(void) {
    double u1 = 0.0, u2 = 0.0;
    // avoid u1 == 0
    while (u1 <= 1e-12) {
        u1 = (rand() + 1.0) / ((double)RAND_MAX + 2.0);
        u2 = (rand() + 1.0) / ((double)RAND_MAX + 2.0);
    }
    double r = sqrt(-2.0 * log(u1));
    double theta = 2.0 * M_PI * u2;
    return r * cos(theta);
}

int main(void) {
    srand((unsigned)time(NULL));

    // Allocate arrays
    double *A = (double*) malloc((size_t)N * N * sizeof(double));
    if (!A) { perror("malloc A"); return 1; }

    // FFTW complex input/output
    fftw_complex *in  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (size_t)N * N);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (size_t)N * N);
    if (!in || !out) { perror("fftw_malloc"); return 1; }

    // Fill A with N(1,1) and copy to complex input (imag = 0)
    for (size_t i = 0; i < (size_t)N * N; ++i) {
        double val = 1.0 + randn();     // mean 1, std 1
        A[i] = val;
        in[i][0] = val;                 // real
        in[i][1] = 0.0;                 // imag
    }

    // Plan and execute 2D C2C FFT (unnormalized forward transform)
    fftw_plan plan = fftw_plan_dft_2d(N, N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    if (!plan) { fprintf(stderr, "FFTW plan creation failed\n"); return 1; }
    fftw_execute(plan);

    // Simple sanity print: C[0,0] (DC component) and a neighbor
    // Note: row-major index (r,c) -> r*N + c
    size_t idx00 = 0;
    size_t idx01 = 1;

    printf("C[0,0] = %.6e + %.6ei\n", out[idx00][0], out[idx00][1]);
    printf("C[0,1] = %.6e + %.6ei\n", out[idx01][0], out[idx01][1]);

    // (Optional) Save A and C to binary for later steps
    FILE *fa = fopen("A.bin", "wb");
    if (fa) { fwrite(A, sizeof(double), (size_t)N * N, fa); fclose(fa); }

    FILE *fc = fopen("C.bin", "wb");
    if (fc) {
        fwrite(out, sizeof(fftw_complex), (size_t)N * N, fc);
        fclose(fc);
    }

    // Cleanup
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    free(A);

    return 0;
}
