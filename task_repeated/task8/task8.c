// test_daxpy.c  — single file: implementation + tests
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stddef.h>

/* --------- Implementation: d = a*x + y --------- */
void daxpy(double a, const double *x, const double *y, double *d, size_t n) {
    for (size_t i = 0; i < n; ++i) d[i] = a * x[i] + y[i];
}

/* --------- Tiny test helpers --------- */
static int almost_equal(double a, double b, double atol, double rtol) {
    double diff = fabs(a - b);
    double tol  = atol + rtol * fmax(fabs(a), fabs(b));
    if (isnan(a) || isnan(b)) return (isnan(a) && isnan(b));
    return diff <= tol;
}
static int vec_equal(const double *a, const double *b, size_t n,
                     double atol, double rtol) {
    for (size_t i = 0; i < n; ++i)
        if (!almost_equal(a[i], b[i], atol, rtol)) return 0;
    return 1;
}
#define ASSERT_OK(cond, msg) do{ if(!(cond)){ fprintf(stderr,"FAIL: %s\n", msg); return 0; } }while(0)
#define TEST(name) static int name(void)

/* --------- Tests --------- */
TEST(test_small_known) {
    const double a = 3.0;
    const double x[] = {0.1, 0.1, 0.1};
    const double y[] = {7.1, 7.1, 7.1};
    double d[3];
    const double ref[] = {7.4, 7.4, 7.4};
    daxpy(a, x, y, d, 3);
    ASSERT_OK(vec_equal(d, ref, 3, 1e-15, 1e-15), "small_known mismatch");
    return 1;
}
TEST(test_zero_scalar) {
    const double a = 0.0;
    const double x[] = {1.0, 2.0, 3.0};
    const double y[] = {4.0, 5.0, 6.0};
    double d[3];
    daxpy(a, x, y, d, 3);
    ASSERT_OK(vec_equal(d, y, 3, 0.0, 0.0), "zero_scalar mismatch");
    return 1;
}
TEST(test_zero_vectors) {
    const size_t n = 5;
    const double a = 2.0;
    double x[5] = {0}, y[5] = {0}, d[5];
    daxpy(a, x, y, d, n);
    for (size_t i = 0; i < n; ++i) ASSERT_OK(d[i] == 0.0, "zero_vectors nonzero");
    return 1;
}
/* In-place on y (BLAS-style): y := a*x + y */
TEST(test_inplace_y) {
    const size_t n = 4;
    const double a = 2.5;
    double x[4] = {1,2,3,4};
    double y[4] = {10,20,30,40};
    double ref[4];
    for (size_t i = 0; i < n; ++i) ref[i] = a*x[i] + y[i];
    daxpy(a, x, y, y, n);
    ASSERT_OK(vec_equal(y, ref, n, 1e-15, 1e-15), "inplace_y mismatch");
    return 1;
}
TEST(test_random_repro) {
    const size_t n = 1000;
    const double a = 1.7;
    double *x = malloc(n*sizeof(double));
    double *y = malloc(n*sizeof(double));
    double *d = malloc(n*sizeof(double));
    double *ref = malloc(n*sizeof(double));
    ASSERT_OK(x && y && d && ref, "malloc failed");

    unsigned int seed = 123u;                 /* simple LCG for reproducibility */
    for (size_t i = 0; i < n; ++i) {
        seed = 1664525u*seed + 1013904223u;
        double rx = ((seed >> 8) / (double)0x01000000) * 20.0 - 10.0;
        seed = 1664525u*seed + 1013904223u;
        double ry = ((seed >> 8) / (double)0x01000000) * 20.0 - 10.0;
        x[i] = rx; y[i] = ry; ref[i] = a*rx + ry;
    }
    daxpy(a, x, y, d, n);
    int ok = vec_equal(d, ref, n, 1e-12, 1e-12);
    free(x); free(y); free(d); free(ref);
    ASSERT_OK(ok, "random_repro mismatch");
    return 1;
}
TEST(test_extremes) {
    const double a = 3.0;
    const double x[] = {1e308, -1e308, 1e-308, -1e-308};
    const double y[] = {1e308,  1e308, 1e-308,  1e-308};
    double d[4], ref[4];
    for (size_t i = 0; i < 4; ++i) ref[i] = a*x[i] + y[i];
    daxpy(a, x, y, d, 4);
    for (size_t i = 0; i < 4; ++i) {
        if (isinf(ref[i]) || isnan(ref[i])) {
            ASSERT_OK((isinf(d[i]) && (signbit(d[i]) == signbit(ref[i]))) ||
                      (isnan(d[i]) && isnan(ref[i])),
                      "extremes inf/nan mismatch");
        } else {
            ASSERT_OK(almost_equal(d[i], ref[i], 0, 0), "extremes finite mismatch");
        }
    }
    return 1;
}

/* --------- Runner --------- */
int main(void) {
    struct { const char* name; int (*fn)(void); } tests[] = {
        {"test_small_known",  test_small_known},
        {"test_zero_scalar",  test_zero_scalar},
        {"test_zero_vectors", test_zero_vectors},
        {"test_inplace_y",    test_inplace_y},
        {"test_random_repro", test_random_repro},
        {"test_extremes",     test_extremes},
    };
    int passed = 1;
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        int ok = tests[i].fn();
        printf("%-20s : %s\n", tests[i].name, ok ? "OK" : "FAIL");
        passed &= ok;
    }
    if (!passed) { fprintf(stderr, "\nSome tests FAILED.\n"); return 1; }
    printf("\nAll tests passed.\n");
    return 0;
}
