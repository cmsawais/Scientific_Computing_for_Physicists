#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Function definition f(x) = e^x * cos(x)
double f(double x) {
    return exp(x) * cos(x);
}

// Numerical integration using trapezoidal rule
double trapezoidal(double x_inf, double x_sup, int N) {
    double h = (x_sup - x_inf) / (double)(N - 1);
    double integral = 0.0;

    for (int i = 1; i < N - 1; i++) {
        integral += f(x_inf + i * h);
    }

    integral += (f(x_inf) + f(x_sup)) / 2.0;
    integral *= h;

    return integral;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s N x_inf x_sup\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    double x_inf = atof(argv[2]);
    double x_sup = atof(argv[3]);

    // Write sampled values to file
    FILE *fp = fopen("task4_output.txt", "w");
    if (!fp) {
        perror("File opening failed");
        return 1;
    }

    double h = (x_sup - x_inf) / (double)(N - 1);
    for (int i = 0; i < N; i++) {
        double x = x_inf + i * h;
        fprintf(fp, "%.8f %.8f\n", x, f(x));
    }
    fclose(fp);

    // Compute numerical integral
    double I = trapezoidal(0.0, M_PI/2.0, N);

    // Analytic solution
    double I_true = (exp(M_PI/2.0) - 1.0) / 2.0;

    // Relative error
    double eps_rel = (I / I_true) - 1.0;

    printf("Numerical Integral I  = %.16f\n", I);
    printf("Analytic Solution     = %.16f\n", I_true);
    printf("Relative Error        = %.16e\n", eps_rel);

    return 0;
}
// OutPut on Terminal //
// Numerical Integral I  = 1.9052374957512166
// Analytic Solution     = 1.9052386904826757
// Relative Error        = -6.2707705084097398e-07
