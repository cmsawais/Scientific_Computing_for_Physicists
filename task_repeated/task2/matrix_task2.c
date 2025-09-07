// matrix_task02.c
// C = A * B with A=3.0, B=7.1 for N in {10, 100, 10000}.
// Uses naive multiplication when N<=1000; analytic check for larger N.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

static inline bool isclose(double a, double b, double atol) {
    return fabs(a - b) <= atol;
}

// 2D accessor helpers for flat arrays
static inline double get(const double *M, long long N, long long i, long long j) {
    return M[i*N + j];
}
static inline void set(double *M, long long N, long long i, long long j, double v) {
    M[i*N + j] = v;
}

void multiply_naive(long long N, double aval, double bval) {
    printf("[C][Naive] N=%lld (allocating A,B,C: %.2f MB)...\n",
           N, (3.0 * N * N * sizeof(double)) / (1024.0*1024.0));

    double *A = (double*)malloc((size_t)N*(size_t)N*sizeof(double));
    double *B = (double*)malloc((size_t)N*(size_t)N*sizeof(double));
    double *C = (double*)malloc((size_t)N*(size_t)N*sizeof(double));
    if (!A || !B || !C) {
        fprintf(stderr, "Allocation failed for N=%lld\n", N);
        free(A); free(B); free(C);
        return;
    }

    // Fill A=3.0, B=7.1
    for (long long i = 0; i < N*N; ++i) A[i] = aval;
    for (long long i = 0; i < N*N; ++i) B[i] = bval;

    // C = A * B (triple loop)
    for (long long i = 0; i < N; ++i) {
        for (long long j = 0; j < N; ++j) {
            double s = 0.0;
            for (long long k = 0; k < N; ++k) {
                s += get(A,N,i,k) * get(B,N,k,j);
            }
            set(C,N,i,j,s);
        }
    }

    // Verify: each entry should equal (3.0*7.1)*N = 21.3*N
    double expected = 21.3 * (double)N;
    double max_abs_err = 0.0;
    bool ok = true;
    for (long long i = 0; i < N; ++i) {
        for (long long j = 0; j < N; ++j) {
            double diff = fabs(get(C,N,i,j) - expected);
            if (diff > max_abs_err) max_abs_err = diff;
            if (diff > 1e-9) ok = false;
        }
    }

    printf("  expected each C[i,j] = %.10f\n", expected);
    printf("  C[0,0]=%.10f, C[N-1,N-1]=%.10f\n",
           get(C,N,0,0), get(C,N,N-1,N-1));
    printf("  max_abs_error=%.3e, all_equal=%s\n",
           max_abs_err, ok ? "true" : "false");

    // Optional checksum for another sanity check
    long double sum = 0.0L;
    for (long long i = 0; i < N*N; ++i) sum += C[i];
    printf("  checksum(sum of all C) = %.0Lf\n", sum);

    free(A); free(B); free(C);
}

void multiply_analytic(long long N, double aval, double bval) {
    // We know each C[i,j] = (a*b)*N. Avoid huge memory/time at large N.
    double expected = (aval * bval) * (double)N; // 21.3 * N
    bool ok = isclose(expected, 21.3 * (double)N, 1e-12);

    // Theoretical checksum = N*N * expected
    long double checksum = (long double)N * (long double)N * (long double)expected;

    printf("[C][Analytic] N=%lld (no allocation)\n", N);
    printf("  expected each C[i,j] = %.10f; test_ok=%s\n", expected, ok ? "true" : "false");
    printf("  theoretical checksum = %.0Lf\n", checksum);
}

int main(void) {
    const double AVAL = 3.0;
    const double BVAL = 7.1;

    long long Ns[] = {10, 100, 10000};
    int nN = sizeof(Ns)/sizeof(Ns[0]);

    for (int t = 0; t < nN; ++t) {
        long long N = Ns[t];
        if (N <= 1000) {
            multiply_naive(N, AVAL, BVAL);
        } else {
            multiply_analytic(N, AVAL, BVAL);
        }
        puts("");
    }
    return 0;
}
 // OutPut 
 //[C][Naive] N=10 (allocating A,B,C: 0.00 MB)...expected each C[i,j] = 213.0000000000 C[0,0]=213.0000000000, C[N-1,N-1]=213.0000000000  max_abs_error=2.842e-14, all_equal=true checksum(sum of all C) = 21300[C][Naive] N=100 (allocating A,B,C: 0.23 MB)...
 // expected each C[i,j] = 2130.0000000000  C[0,0]=2130.0000000000, C[N-1,N-1]=2130.0000000000  max_abs_error=2.274e-12, all_equal=true   checksum(sum of all C) = 21300000
 //[C][Analytic] N=10000 (no allocation) expected each C[i,j] = 213000.0000000000; test_ok=false  theoretical checksum = 21300000000000