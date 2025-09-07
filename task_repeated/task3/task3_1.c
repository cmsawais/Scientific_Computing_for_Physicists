// gen_vectors.c
// Usage:
//   ./gen_vectors N filename_prefix
// Example:
//   ./gen_vectors 10 "/path/to/my/outputdir/vector_"
// Produces:
//   /path/to/my/outputdir/vector_N10_x.dat
//   /path/to/my/outputdir/vector_N10_y.dat

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

static long long parse_ll(const char *s) {
    char *end = NULL;
    errno = 0;
    long long v = strtoll(s, &end, 10);
    if (errno || end == s || *end != '\0') {
        fprintf(stderr, "Invalid integer: '%s'\n", s);
        exit(1);
    }
    return v;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s N filename_prefix\n", argv[0]);
        fprintf(stderr, "Example: %s 10 /path/to/my/outputdir/vector_\n", argv[0]);
        return 1;
    }

    long long N = parse_ll(argv[1]);
    if (N <= 0) {
        fprintf(stderr, "N must be a positive integer (got %lld)\n", N);
        return 1;
    }
    const char *prefix = argv[2];

    // Build output filenames
    char fx[2048], fy[2048];
    int nx = snprintf(fx, sizeof(fx), "%sN%lld_x.dat", prefix, N);
    int ny = snprintf(fy, sizeof(fy), "%sN%lld_y.dat", prefix, N);
    if (nx < 0 || ny < 0 || nx >= (int)sizeof(fx) || ny >= (int)sizeof(fy)) {
        fprintf(stderr, "Output path too long\n");
        return 1;
    }

    // Open files
    FILE *fpx = fopen(fx, "w");
    if (!fpx) { perror(fx); return 1; }
    FILE *fpy = fopen(fy, "w");
    if (!fpy) { perror(fy); fclose(fpx); return 1; }

    // Optional: larger buffers for speed on big N
    static char bufX[1<<16], bufY[1<<16];
    setvbuf(fpx, bufX, _IOFBF, sizeof(bufX));
    setvbuf(fpy, bufY, _IOFBF, sizeof(bufY));

    // Write vectors: x = 0.1, y = 7.1 (text, one number per line)
    const double xval = 0.1, yval = 7.1;
    for (long long i = 0; i < N; ++i) {
        // Use %.17g to preserve double precision in text form
        if (fprintf(fpx, "%.17g\n", xval) < 0) { perror("write x"); break; }
        if (fprintf(fpy, "%.17g\n", yval) < 0) { perror("write y"); break; }
    }

    if (fclose(fpx) != 0) { perror(fx); return 1; }
    if (fclose(fpy) != 0) { perror(fy); return 1; }

    printf("Wrote:\n  %s\n  %s\n", fx, fy);
    return 0;
}
