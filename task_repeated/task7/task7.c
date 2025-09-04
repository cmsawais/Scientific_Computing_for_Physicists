#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>   // for memset
#include <time.h>

// ---------- DAXPY IMPLEMENTATION ----------
int daxpy(double a, const double *x, const double *y, double *out, size_t n) {
    // Validate pointers and n
    if ((n > 0) && (!x || !y || !out)) return -1;
    // Allow n == 0 as a no-op (success)
    for (size_t i = 0; i < n; ++i) {
        out[i] = a * x[i] + y[i];
    }
    return 0;
}

// ---------- NUMERICAL HELPERS ----------
static inline double dabs(double v) { return v < 0 ? -v : v; }

bool almost_equal(double a, double b, double rtol, double atol) {
    // Handles NaN/Inf: exact same bit pattern is not required, but follow common numeric practice
    if (isnan(a) && isnan(b)) return true;
    if (isinf(a) || isinf(b)) return a == b; // both must be same infinity
    double diff = dabs(a - b);
    double tol  = atol + rtol * dabs(b);
    return diff <= tol;
}

bool array_allclose(const double *a, const double *b, size_t n, double rtol, double atol) {
    for (size_t i = 0; i < n; ++i) {
        if (!almost_equal(a[i], b[i], rtol, atol)) return false;
    }
    return true;
}

bool array_equal_exact(const double *a, const double *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ---------- ASSERTIONS ----------
#define ASSERT_TRUE(cond, name) do { \
    if (cond) { printf("[PASS] %s\n", name); } \
    else      { printf("[FAIL] %s\n", name); ++g_failed; } \
} while(0)

#define ASSERT_ZERO_RET(expr, name) do { \
    int __rc = (expr); \
    if (__rc == 0) { printf("[PASS] %s\n", name); } \
    else           { printf("[FAIL] %s (rc=%d)\n", name, __rc); ++g_failed; } \
} while(0)

static int g_failed = 0;

// ---------- TESTS ----------
void test_basic_case(void) {
    const double a = 3.0;
    double x[3] = {0.1, 0.2, 0.3};
    double y[3] = {7.1, 7.2, 7.3};
    double expected[3] = {7.4, 7.8, 8.2};
    double out[3] = {0};

    int rc = daxpy(a, x, y, out, 3);
    bool ok_rc = (rc == 0);
    bool ok_eq = array_allclose(out, expected, 3, 1e-12, 0.0);

    if (!ok_rc) printf("  rc=%d (expected 0)\n", rc);
    if (!ok_eq) printf("  mismatch out=[%g,%g,%g]\n", out[0], out[1], out[2]);
    ASSERT_TRUE(ok_rc && ok_eq, "test_basic_case");
}

void test_zeros(void) {
    const double a = 3.0;
    double x[5] = {0,0,0,0,0};
    double y[5] = {0,0,0,0,0};
    double expected[5] = {0,0,0,0,0};
    double out[5] = {1,1,1,1,1}; // prefill to ensure function writes

    int rc = daxpy(a, x, y, out, 5);
    bool ok = (rc == 0) && array_equal_exact(out, expected, 5);
    ASSERT_TRUE(ok, "test_zeros");
}

void test_random_input(void) {
    // Deterministic uniform random in [-1, 1]
    srand(42);
    const size_t n = 10;
    const double a = 2.0;
    double x[n], y[n], expected[n], out[n];

    for (size_t i = 0; i < n; ++i) {
        double rx = (double)rand() / (double)RAND_MAX;  // [0,1]
        double ry = (double)rand() / (double)RAND_MAX;
        x[i] = 2.0 * rx - 1.0; // [-1,1]
        y[i] = 2.0 * ry - 1.0;
        expected[i] = a * x[i] + y[i];
    }

    int rc = daxpy(a, x, y, out, n);
    bool ok = (rc == 0) && array_allclose(out, expected, n, 1e-12, 0.0);
    if (!ok) {
        printf("  expected vs out (first 3): e=%g %g %g  o=%g %g %g\n",
               expected[0], expected[1], expected[2], out[0], out[1], out[2]);
    }
    ASSERT_TRUE(ok, "test_random_input");
}

void test_invalid_args(void) {
    // n>0 but NULL pointers should return error
    double dummy = 0.0;
    int rc1 = daxpy(1.0, NULL, &dummy, &dummy, 1);
    int rc2 = daxpy(1.0, &dummy, NULL, &dummy, 1);
    int rc3 = daxpy(1.0, &dummy, &dummy, NULL, 1);
    bool ok = (rc1 != 0) && (rc2 != 0) && (rc3 != 0);
    ASSERT_TRUE(ok, "test_invalid_args");
}

void test_empty_arrays(void) {
    // n==0 should be a no-op but still succeed
    int rc = daxpy(2.0, NULL, NULL, NULL, 0);
    ASSERT_TRUE(rc == 0, "test_empty_arrays");
}

void test_nan_propagation(void) {
    const size_t n = 3;
    const double a = 2.0;
    double x[3] = {1.0, NAN, 3.0};
    double y[3] = {4.0, 5.0, 6.0};
    double out[3] = {0};

    int rc = daxpy(a, x, y, out, n);
    bool ok_rc = (rc == 0);
    bool ok_nan = isnan(out[1]);
    bool ok_edges = almost_equal(out[0], 6.0, 1e-12, 0.0) &&
                    almost_equal(out[2], 12.0, 1e-12, 0.0);
    ASSERT_TRUE(ok_rc && ok_nan && ok_edges, "test_nan_propagation");
}

// --- Optional: Uncomment to simulate a failing test 
 void test_intentional_fail(void) {
     const size_t n = 3;
     const double a = 1.0;
     double x[3] = {1.0, 2.0, 3.0};
     double y[3] = {4.0, 5.0, 6.0};
     double out[3] = {0}, expected[3] = {999, 999, 999}; // wrong on purpose
     int rc = daxpy(a, x, y, out, n);
     bool ok = (rc == 0) && array_equal_exact(out, expected, n);
     ASSERT_TRUE(ok, "test_intentional_fail");
 }

// ---------- MAIN ----------
int main(void) {
    printf("Running C DAXPY tests...\n");

    test_basic_case();
    test_zeros();
    test_random_input();
    test_invalid_args();
    test_empty_arrays();
    test_nan_propagation();
    // test_intentional_fail(); // <- enable to see a failing test

    if (g_failed == 0) {
        printf("All tests passed.\n");
        return 0;
    } else {
        printf("%d test(s) failed.\n", g_failed);
        return 1;
    }
}
