// task9_openmp.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

static double now_sec(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec*1e-9;
}

int main(int argc, char** argv){
    size_t N = (argc > 1) ? strtoull(argv[1], NULL, 10) : 5000000ULL;
    double *x = aligned_alloc(64, N*sizeof(double));
    double *y = aligned_alloc(64, N*sizeof(double));
    double *d = aligned_alloc(64, N*sizeof(double));
    if(!x||!y||!d){ fprintf(stderr,"alloc failed\n"); return 1; }

    const double xval = 0.1, yval = 7.1, expected = xval + yval;

    // init
    #pragma omp parallel for schedule(static)
    for(size_t i=0;i<N;++i){ x[i]=xval; y[i]=yval; }

    // --- serial baseline ---
    double t0 = now_sec();
    for(size_t i=0;i<N;++i) d[i] = x[i] + y[i];
    double t1 = now_sec();

    double max_abs_err = 0.0;
    for(size_t i=0;i<N;++i){
        double e = fabs(d[i]-expected);
        if(e>max_abs_err) max_abs_err=e;
    }
    printf("[Serial]  N=%zu  time=%.6f s  BW≈%.2f GB/s  max|err|=%.3e\n",
           N, t1-t0, (3.0*N*sizeof(double))/(t1-t0)/1e9, max_abs_err);

    // --- OpenMP parallel ---
    double t2 = now_sec();
    #pragma omp parallel for schedule(static)
    for(size_t i=0;i<N;++i) d[i] = x[i] + y[i];
    double t3 = now_sec();

    max_abs_err = 0.0;
    #pragma omp parallel for reduction(max:max_abs_err)
    for(size_t i=0;i<N;++i){
        double e = fabs(d[i]-expected);
        if(e>max_abs_err) max_abs_err=e;
    }

    int threads = 1;
    #pragma omp parallel
    { #pragma omp master threads=threads = omp_get_num_threads(); }

    double speedup = (t1-t0)/(t3-t2);
    printf("[OpenMP]  N=%zu  thr=%d  time=%.6f s  speedup=%.2f  BW≈%.2f GB/s  max|err|=%.3e\n",
           N, threads, t3-t2, speedup, (3.0*N*sizeof(double))/(t3-t2)/1e9, max_abs_err);

    // (Optional) reduction sum(d) with OpenMP
    double sumd = 0.0;
    #pragma omp parallel for reduction(+:sumd)
    for(size_t i=0;i<N;++i) sumd += d[i];
    double expected_sum = expected * (double)N;
    printf("[OpenMP]  reduction sum(d)=%.6e (expected %.6e)  abs_err=%.3e\n",
           sumd, expected_sum, fabs(sumd-expected_sum));

    free(x); free(y); free(d);
    return 0;
}
