#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

static inline double dabs(double v){ return v < 0 ? -v : v; }

bool arrays_allclose(const double *a, const double *b, size_t n, double rtol, double atol){
    for(size_t i=0;i<n;++i){
        double diff = dabs(a[i] - b[i]);
        double tol  = atol + rtol * dabs(b[i]);
        if (diff > tol) return false;
    }
    return true;
}

double sum_array(const double *a, size_t n){
    double s = 0.0;
    for(size_t i=0;i<n;++i) s += a[i];
    return s;
}

// Reference: single-loop daxpy: d[i] = a*x[i] + y[i]
void daxpy_single(double a, const double *x, const double *y, double *d, size_t n){
    for(size_t i=0;i<n;++i) d[i] = a*x[i] + y[i];
}

// Chunked version
// Also fills partial_chunk_sum with the sum of each chunk's d-values.
void daxpy_chunked(double a, const double *x, const double *y,
                   double *d, size_t n, size_t chunk_size, double *partial_chunk_sum){
    if (chunk_size == 0) {
        fprintf(stderr, "chunk_size must be >= 1\n");
        exit(1);
    }
    size_t chunks = (n + chunk_size - 1) / chunk_size; // ceiling
    for(size_t k=0;k<chunks;++k){
        size_t start = k * chunk_size;
        size_t end   = start + chunk_size;
        if (end > n) end = n;

        double local_sum = 0.0;
        for(size_t i=start;i<end;++i){
            d[i] = a*x[i] + y[i];
            local_sum += d[i];
        }
        partial_chunk_sum[k] = local_sum;
    }
}

int main(void){
    // Example sizes (change as you like)
    size_t n = 100;         // total elements
    size_t chunk_size = 8;  // chunk size

    // Allocate arrays
    double *x = (double*)malloc(n * sizeof(double));
    double *y = (double*)malloc(n * sizeof(double));
    double *d_single = (double*)malloc(n * sizeof(double));
    double *d_chunk  = (double*)malloc(n * sizeof(double));

    if(!x || !y || !d_single || !d_chunk){
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    // Fill x, y with some reproducible data
    srand(42);
    for(size_t i=0;i<n;++i){
        // values in [-1, 1]
        double rx = (double)rand() / (double)RAND_MAX;
        double ry = (double)rand() / (double)RAND_MAX;
        x[i] = 2.0*rx - 1.0;
        y[i] = 2.0*ry - 1.0;
    }
    double a = 2.0;

    // 1) Single-loop baseline
    daxpy_single(a, x, y, d_single, n);

    // 2) Chunked version + partial sums
    size_t chunks = (n + chunk_size - 1) / chunk_size;
    double *partial_chunk_sum = (double*)malloc(chunks * sizeof(double));
    if(!partial_chunk_sum){
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    daxpy_chunked(a, x, y, d_chunk, n, chunk_size, partial_chunk_sum);

    // A) Check element-wise equality between single and chunked results
    bool same = arrays_allclose(d_single, d_chunk, n, 1e-12, 0.0);
    printf("[CHECK] d(single) == d(chunked)? %s\n", same ? "YES" : "NO");

    // B) Check sum(partial_chunk_sum) == sum(d_single)
    double sum_partials = sum_array(partial_chunk_sum, chunks);
    double sum_single   = sum_array(d_single, n);
    double sum_diff     = fabs(sum_partials - sum_single);
    printf("[CHECK] sum(partials)=%.15f  sum(d_single)=%.15f  |diff|=%.3e  => %s\n",
           sum_partials, sum_single, sum_diff,
           (sum_diff <= 1e-12 * (1.0 + fabs(sum_single))) ? "MATCH" : "MISMATCH");

    // Cleanup
    free(partial_chunk_sum);
    free(d_chunk);
    free(d_single);
    free(y);
    free(x);
    return same && (sum_diff <= 1e-12 * (1.0 + fabs(sum_single))) ? 0 : 1;
}
// Output: [CHECK] d(single) == d(chunked)? YES
// [CHECK] sum(partials)=13.552160067275240  sum(d_single)=13.552160067275242  |diff|=1.776e-15  => MATCH