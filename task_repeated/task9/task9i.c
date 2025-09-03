// task9_serial.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

static double now_sec(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec*1e-9;
}

int main(int argc, char** argv){
    size_t N = (argc > 1) ? strtoull(argv[1], NULL, 10) : 5000000ULL; // few 10^6
    double *x = aligned_alloc(64, N*sizeof(double));
    double *y = aligned_alloc(64, N*sizeof(double));
    double *d = aligned_alloc(64, N*sizeof(double));
    if(!x||!y||!d){ fprintf(stderr,"alloc failed\n"); return 1; }

    const double xval = 0.1, yval = 7.1, expected = xval + yval;

    // init
    for(size_t i=0;i<N;++i){ x[i]=xval; y[i]=yval; }

    // warmup
    for(size_t i=0;i<1000 && i<N;++i) d[i]=x[i]+y[i];

    // timed run
    double t0 = now_sec();
    for(size_t i=0;i<N;++i) d[i] = x[i] + y[i];
    double t1 = now_sec();

    // quick correctness
    double max_abs_err = 0.0;
    for(size_t i=0;i<N;++i){
        double e = fabs(d[i]-expected);
        if(e>max_abs_err) max_abs_err=e;
    }

    printf("[Serial] N=%zu  time=%.6f s  BW≈%.2f GB/s  max|err|=%.3e\n",
           N, t1-t0,
           (3.0*N*sizeof(double))/(t1-t0)/1e9, // x,y read + d write
           max_abs_err);

    free(x); free(y); free(d);
    return 0;
}
