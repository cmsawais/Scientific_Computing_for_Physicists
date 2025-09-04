// task9_mpi.c
// This program scatters x,y across ranks, computes local d = x + y,

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

static inline int approx_equal(double a, double b, double rtol, double atol) {
    double diff = fabs(a - b);
    return diff <= (atol + rtol * fabs(b));
}

// fill arrays deterministically like in OpenMP program
static void fill_xy(double *x, double *y, size_t N) {
    unsigned long long seed = 42;
    for (size_t i = 0; i < N; ++i) {
        seed = seed * 2862933555777941757ULL + 3037000493ULL;
        double rx = (double)(seed >> 33) / (double)(1ULL<<31);
        x[i] = rx - 1.0;
        seed = seed * 2862933555777941757ULL + 3037000493ULL;
        double ry = (double)(seed >> 33) / (double)(1ULL<<31);
        y[i] = ry - 1.0;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    size_t N = (argc > 1) ? strtoull(argv[1], NULL, 10) : (size_t)2000000; // default: 2M
    if (size > 4) {
        if (rank == 0) fprintf(stderr, "Please do not spawn more than 4 tasks for this assignment.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    double t0, t1, t2, t3;
    double serial_time = 0.0, mpi_time = 0.0;

    // Rank 0 allocates full arrays, others only receive slices
    double *x = NULL, *y = NULL, *d_serial = NULL, *d_gather = NULL;
    if (rank == 0) {
        x = (double*) aligned_alloc(64, N * sizeof(double));
        y = (double*) aligned_alloc(64, N * sizeof(double));
        d_serial = (double*) aligned_alloc(64, N * sizeof(double));
        d_gather = (double*) aligned_alloc(64, N * sizeof(double));
        if (!x || !y || !d_serial || !d_gather) {
            fprintf(stderr, "Allocation failed on rank 0.\n");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
        fill_xy(x, y, N);

        // Serial baseline on rank 0 (for timing & correctness)
        t0 = MPI_Wtime();
        for (size_t i = 0; i < N; ++i) d_serial[i] = x[i] + y[i];
        t1 = MPI_Wtime();
        serial_time = t1 - t0;
    }

    // Compute scatter counts and displacements
    int *counts = (int*) malloc(size * sizeof(int));
    int *displs = (int*) malloc(size * sizeof(int));
    if (!counts || !displs) {
        fprintf(stderr, "Allocation of counts/displs failed.\n");
        MPI_Abort(MPI_COMM_WORLD, 3);
    }
    size_t base = N / size;
    size_t rem  = N % size;
    size_t offset = 0;
    for (int r = 0; r < size; ++r) {
        size_t chunk = base + (r < rem ? 1 : 0);
        counts[r] = (int)chunk;
        displs[r] = (int)offset;
        offset += chunk;
    }

    size_t local_n = (size_t)counts[rank];
    double *x_local = (double*) aligned_alloc(64, local_n * sizeof(double));
    double *y_local = (double*) aligned_alloc(64, local_n * sizeof(double));
    double *d_local = (double*) aligned_alloc(64, local_n * sizeof(double));
    if (!x_local || !y_local || !d_local) {
        fprintf(stderr, "Allocation failed on rank %d.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 4);
    }

    // Scatterv x and y
    // Note: MPI_Scatterv counts/displs are in units of elements, not bytes.
    t2 = MPI_Wtime();
    MPI_Scatterv(x, counts, displs, MPI_DOUBLE, x_local, counts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(y, counts, displs, MPI_DOUBLE, y_local, counts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Local compute
    for (size_t i = 0; i < local_n; ++i) d_local[i] = x_local[i] + y_local[i];

    // (Optional) parallel reduction of sum(d)
    double local_sum = 0.0;
    for (size_t i = 0; i < local_n; ++i) local_sum += d_local[i];
    double global_sum = 0.0;
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // Gather results to rank 0
    MPI_Gatherv(d_local, counts[rank], MPI_DOUBLE, d_gather, counts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    t3 = MPI_Wtime();
    mpi_time = t3 - t2;

    if (rank == 0) {
        // Check correctness
        double max_abs_diff = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double diff = fabs(d_serial[i] - d_gather[i]);
            if (diff > max_abs_diff) max_abs_diff = diff;
        }
        printf("[CHECK] max |d_serial - d_mpi| = %.3e => %s\n",
               max_abs_diff, (max_abs_diff <= 1e-12 ? "OK" : "MISMATCH"));

        // Reduction check
        double serial_sum = 0.0;
        for (size_t i = 0; i < N; ++i) serial_sum += d_serial[i];
        printf("[CHECK] sum(d) serial=%.15f  mpi=%.15f  => %s\n",
               serial_sum, global_sum,
               approx_equal(serial_sum, global_sum, 1e-12, 0.0) ? "MATCH" : "MISMATCH");

        // Timing
        printf("[TIME] serial (rank0): %.6f s | mpi (np=%d): %.6f s\n",
               serial_time, size, mpi_time);
    }

    free(d_local); free(y_local); free(x_local);
    free(displs); free(counts);
    if (rank == 0) { free(d_gather); free(d_serial); free(y); free(x); }

    MPI_Finalize();
    return 0;
}

// I was unable to compile and run it here in cluster because i don't have rights to install mpi compiler wrapper. I also tried cluster module system but it didn't work, so finally i used Colab to compile and run it
// here is the output form Colab compilation and execution. 
//[CHECK] max |d_serial - d_mpi| = 0.000e+00 => OK
//[CHECK] sum(d) serial=-3999759.056873759720474  mpi=-3999759.056873759720474  => MATCH
//[TIME] serial (rank0): 0.060795 s | mpi (np=2): 0.185134 s

//[CHECK] max |d_serial - d_mpi| = 0.000e+00 => OK
//[CHECK] sum(d) serial=-3999759.056873759720474  mpi=-3999759.056873759720474  => MATCH
//[TIME] serial (rank0): 0.052945 s | mpi (np=4): 0.135739 s

// Link to code: https://colab.research.google.com/drive/1DuMXUo2on42J2qczuVAzGhb1aj4NBiW1?usp=sharing 