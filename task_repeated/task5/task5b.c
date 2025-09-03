// task5b.c
#include <stdio.h>
#include <gsl/gsl_statistics_double.h>  // gsl_stats_mean

int main(void) {
    // Vettore dell'esercizio
    double vec[] = {1.0, 1.0e16, -1.0e16, -0.5};
    size_t n = sizeof(vec) / sizeof(vec[0]);

    // mean = gsl_stats_mean(data, stride, n)
    // Per array contigui, stride = 1
    double mean = gsl_stats_mean(vec, 1, n);
    double sum  = mean * (double)n;

    printf("Sum using GSL (gsl_stats_mean * n): %.17g\n", sum);
    return 0;
}
