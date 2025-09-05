// task5d.c — DAXPY with Gaussian vectors and correctness checks
// Build: gcc -O2 -Wall -std=c11 task5d.c -lm -o task5d
// Run:   ./task5d [N] [a] [seed]
//        e.g., ./task5d            (defaults: N=1e6, a=3.0, seed=time)
//              ./task5d 200000 1.0 42

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// ---- Gaussian(0,1) via Box–Muller ----
static double gauss01(void) {
    // u1 in (0,1], u2 in [0,1)
    double u1 = (rand() + 1.0) / (RAND_MAX + 2.0);
    double u2 = (rand() + 0.0) / (RAND_MAX + 1.0);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

// ---- Online stats (Welford) ----
typedef struct { long long n; double mean; double M2; } Welford;

static void w_init(Welford *w){ w->n=0; w->mean=0.0; w->M2=0.0; }
static void w_push(Welford *w, double x){
    w->n++;
    double delta = x - w->mean;
    w->mean += delta / (double)w->n;
    double delta2 = x - w->mean;
    w->M2 += delta * delta2;
}
static double w_mean(const Welford *w){ return w->mean; }
static double w_var_sample(const Welford *w){ return (w->n>1)? w->M2/(w->n-1) : NAN; }

int main(int argc, char **argv){
    // Parameters (all optional): N, a, seed
    long long N   = (argc>1)? atoll(argv[1]) : 1000000LL;
    double a      = (argc>2)? atof(argv[2])  : 3.0;
    unsigned seed = (argc>3)? (unsigned)strtoul(argv[3],NULL,10) : (unsigned)time(NULL);

    srand(seed);

    // Allocate
    double *x = (double*)malloc(sizeof(double)*N);
    double *y = (double*)malloc(sizeof(double)*N);
    double *d = (double*)malloc(sizeof(double)*N);
    if(!x || !y || !d){
        fprintf(stderr,"[ERROR] Allocation failed\n");
        free(x); free(y); free(d);
        return 1;
    }

    // Fill x,y with N(0,1) and compute d = a*x + y
    for(long long i=0;i<N;i++){
        x[i] = gauss01();
        y[i] = gauss01();
        d[i] = a*x[i] + y[i];
    }

    // -------- Test 1: Deterministic (formula consistency) --------
    // Verify d[i] == a*x[i] + y[i] within floating-point tolerance
    double max_abs_err = 0.0, l1_err = 0.0;
    for(long long i=0;i<N;i++){
        double ref = a*x[i] + y[i];
        double e = fabs(d[i] - ref);
        if(e > max_abs_err) max_abs_err = e;
        l1_err += e;
    }

    // -------- Test 2: Statistical (distribution properties) --------
    // If x,y ~ N(0,1) independent, then d ~ N(0, a^2 + 1).
    double var_theory = a*a + 1.0;
    Welford ws; w_init(&ws);
    for(long long i=0;i<N;i++) w_push(&ws, d[i]);

    double mean_hat = w_mean(&ws);
    double var_hat  = w_var_sample(&ws);

    // Standard error of the sample mean: sqrt(Var(d)/N)
    double se_mean = sqrt(var_theory / (double)N);

    // Simple pass/fail checks (practical thresholds)
    int pass_mean = (fabs(mean_hat) <= 3.0*se_mean);              // mean ~ 0 ?
    int pass_var  = (fabs(var_hat - var_theory) <= 0.05*var_theory); // var ~ a^2+1 within ~5%

    // -------- Report --------
    printf("N=%lld, a=%.6f, seed=%u\n", N, a, seed);
    printf("[Deterministic] max |d - (a*x + y)| = %.3e,  L1 total error = %.3e\n",
           max_abs_err, l1_err);
    printf("[Statistical]   mean(d)   = %.6e  (expected 0)\n", mean_hat);
    printf("                var(d)    = %.6e  (expected %.6e)\n", var_hat, var_theory);
    printf("                3*SE(mean)= %.6e  => mean should lie within +/- this band\n", 3.0*se_mean);
    printf("Checks: mean ~ 0 ? %s  |  var ~ a^2+1 ? %s\n",
           pass_mean? "OK":"NO", pass_var? "OK":"NO");

    free(x); free(y); free(d);
    return 0;
}
