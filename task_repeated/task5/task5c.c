// task5c.c
#include <stdio.h>
#include <stddef.h>

double kahan_sum(const double *a, size_t n) {
    double sum = 0.0;
    double c = 0.0; // compensation for lost low-order bits
    for (size_t i = 0; i < n; ++i) {
        double y = a[i] - c;
        double t = sum + y;
        c = (t - sum) - y; // new compensation
        sum = t;
    }
    return sum;
}

double naive_sum(const double *a, size_t n) {
    double s = 0.0;
    for (size_t i = 0; i < n; ++i) s += a[i];
    return s;
}

int main(void) {
    // Vector from the assignment
    double vec[] = {1.0, 1.0e16, -1.0e16, -0.5};
    size_t n = sizeof(vec) / sizeof(vec[0]);

    double s_naive = naive_sum(vec, n);
    double s_kahan = kahan_sum(vec, n);

    // Print with high precision to see differences
    printf("Naive sum:  %.17g\n", s_naive);
    printf("Kahan sum:  %.17g\n", s_kahan);

    return 0;
}
