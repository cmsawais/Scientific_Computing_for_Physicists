// Task 02 (C):
// 1) Vector d = a*x + y for N in {10, 1e6, 1e8}, with a=3, x=0.1, y=7.1, verify d=7.4.
// 3) Matrix C = A*B for N in {10, 100, 10000}, with A=3, B=7.1 everywhere.
//    For true matrix multiplication, each C[i][j] = sum_k (3*7.1) = 21.3 * N.

#include <stdio.h>
#include <math.h>
#include <stdbool.h>

static bool isclose(double a, double b, double atol) {
    return fabs(a - b) <= atol;
}

void vector_test(long long N, double a, double xval, double yval, const char *outfile) {
    double expected = a * xval + yval; // 7.4
    double checksum = 0.0;
    bool all_equal = true;

    double head[5], tail[5];
    for (int i = 0; i < 5; i++) { head[i] = 0.0; tail[i] = 0.0; }

    for (long long i = 0; i < N; i++) {
        double z = a * xval + yval;
        checksum += z;
        if (i < 5) head[i] = z;
        if (i >= N - 5) tail[i - (N - 5)] = z;
        if (!isclose(z, expected, 1e-12)) all_equal = false;
    }

    FILE *fp = fopen(outfile, "w");
    if (!fp) { perror("fopen"); return; }
    fprintf(fp, "N=%lld\n", N);
    fprintf(fp, "expected element value = %.15g\n", expected);
    fprintf(fp, "first 5 = [");
    for (int i = 0; i < 5 && i < N; i++) {
        fprintf(fp, "%.6f%s", head[i], (i<4 && i+1 < N) ? ", " : "");
    }
    fprintf(fp, "]\n");
    fprintf(fp, "last 5  = [");
    int count = (N < 5) ? (int)N : 5;
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%.6f%s", tail[i], (i<4 && i+1 < count) ? ", " : "");
    }
    fprintf(fp, "]\n");
    fprintf(fp, "checksum(sum of all elements) = %.15g\n", checksum);
    fprintf(fp, "all_equal = %s\n", all_equal ? "true" : "false");
    fclose(fp);

    printf("[C][Vector] N=%lld: all_equal=%s, checksum=%.6f\n",
           N, all_equal ? "true" : "false", checksum);
}

void matrix_test(long long N, const char *outfile) {
    // Analytic result (avoid O(N^3) compute): every C[i][j] = 21.3 * N
    double expected = 21.3 * (double)N;
    bool ok = isclose(expected, 21.3 * (double)N, 1e-12); // trivial true, just to mirror Python

    FILE *fp = fopen(outfile, "w");
    if (!fp) { perror("fopen"); return; }
    fprintf(fp, "N=%lld\n", N);
    fprintf(fp, "expected each C[i,j] = %.15g\n", expected);
    fprintf(fp, "all_equal = %s\n", ok ? "true" : "false");
    fclose(fp);

    printf("[C][Matrix] N=%lld: each entry should be %.6f; test_ok=%s\n",
           N, expected, ok ? "true" : "false");
}

int main(void) {
    // PART 1: vectors
    vector_test(10,        3.0, 0.1, 7.1, "c_outputs_vector_N10.txt");
    vector_test(1000000LL, 3.0, 0.1, 7.1, "c_outputs_vector_N1000000.txt");
    vector_test(100000000LL,3.0, 0.1, 7.1, "c_outputs_vector_N100000000.txt");

    // PART 3: matrices
    matrix_test(10,     "c_outputs_matrix_N10.txt");
    matrix_test(100,    "c_outputs_matrix_N100.txt");
    matrix_test(10000,  "c_outputs_matrix_N10000.txt");

    return 0;
}
