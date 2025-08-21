#include <stdio.h>

int main() {
    double a = 3.0;
    int N = 20;
    double x[20], y[20], z[20];

    // initialize vectors
    for (int i = 0; i < N; i++) {
        x[i] = 1.0;
        y[i] = 4.0;
    }

    // compute z = a*x + y
    for (int i = 0; i < N; i++) {
        z[i] = a * x[i] + y[i];
    }

    // write result to file
    FILE *fp = fopen("result_c.txt", "w");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    fprintf(fp, "z = [");
    for (int i = 0; i < N; i++) {
        fprintf(fp, "%.2f", z[i]);
        if (i < N-1) fprintf(fp, ", ");
    }
    fprintf(fp, "]\n");
    fprintf(fp, "Each element of z is: %.2f\n", z[0]);

    fclose(fp);
    return 0;
}
