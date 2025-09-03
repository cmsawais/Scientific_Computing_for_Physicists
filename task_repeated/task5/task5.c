#include <stdio.h>
int main(void) {
    // Vector from the assignment
    double vec[] = {1.0, 1.0e16, -1.0e16, -0.5};
    size_t n = sizeof(vec) / sizeof(vec[0]);

    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sum += vec[i];
    }

    // Print with high precision to see floating-point effects
    printf("Sum (for loop): %.17g\n", sum);
    return 0;
}
