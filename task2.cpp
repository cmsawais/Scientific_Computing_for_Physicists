#include <iostream>
#include <vector>

int main() {
    size_t N = 10;
    float a = 3.0f;

    // Initialize vectors x and y
    std::vector<float> x(N, 0.1f);  // x = [0.1, 0.1, ..., 0.1]
    std::vector<float> y(N, 7.1f);  // y = [7.1, 7.1, ..., 7.1]
    std::vector<float> d(N);       // Result vector d

    // Compute d = a * x + y
    for (size_t i = 0; i < N; ++i) {
        d[i] = a * x[i] + y[i];
    }

    // Print results
    std::cout << "Results for d = a * x + y with N = 10:\n";
    for (size_t i = 0; i < N; ++i) {
        std::cout << "d[" << i << "] = " << d[i] << "\n";
    }

    return 0;
}
