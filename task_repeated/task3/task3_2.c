// task3_2.c
// Usage: ./task3_2 config.ini
// Reads: x_file, y_file, N, a, prefix_output  -> writes <prefix_output>N<N>_d.dat
// Computes d = a*x + y (text files, one value per line)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
  #include <direct.h>
  #define MKDIR(p) _mkdir(p)
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #define MKDIR(p) mkdir(p, 0777)
#endif

typedef struct {
    char x_file[1024];
    char y_file[1024];
    char prefix_output[1024];
    long long N;
    double a; // default 3.0
} cfg_t;

/* --- small utils --- */
static void rstrip(char *s){
    size_t n = strlen(s);
    while (n && (s[n-1]=='\n' || s[n-1]=='\r' || isspace((unsigned char)s[n-1]))) s[--n] = '\0';
}
static void lstrip(char **p){
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

/* mkdir -p */
static int mkdir_p(const char *dir){
    if (!dir || !*dir) return 0;
    char tmp[2048];
    snprintf(tmp, sizeof(tmp), "%s", dir);

    /* strip trailing separators */
    size_t len = strlen(tmp);
    while (len && (tmp[len-1]=='/' || tmp[len-1]=='\\')) tmp[--len] = '\0';
    if (!len) return 0;

    char *p = tmp;
#ifdef _WIN32
    if (len >= 2 && tmp[1] == ':') p = tmp + 2;  // skip "C:"
#endif
    if (*p=='/' || *p=='\\') ++p;                // skip single leading slash

    for (; *p; ++p){
        if (*p=='/' || *p=='\\'){
            char hold = *p; *p = '\0';
            if (MKDIR(tmp) != 0 && errno != EEXIST) { perror(tmp); return -1; }
            *p = hold;
        }
    }
    if (MKDIR(tmp) != 0 && errno != EEXIST) { perror(tmp); return -1; }
    return 0;
}

static void ensure_parent_from_prefix(const char *prefix){
    char path[2048];
    snprintf(path, sizeof(path), "%s", prefix);
    size_t len = strlen(path);
    while (len && (path[len-1]=='/' || path[len-1]=='\\')) path[--len] = '\0';

    char *s1 = strrchr(path, '/');
    char *s2 = strrchr(path, '\\');
    char *sep = (s2 && s2 > s1) ? s2 : s1;
    if (!sep) return;            // no directory part
    *sep = '\0';
    if (mkdir_p(path) != 0){
        fprintf(stderr, "Failed to create directory: %s\n", path);
        exit(1);
    }
}

/* parse key=value config */
static void parse_cfg(const char *fname, cfg_t *c){
    FILE *fp = fopen(fname, "r");
    if (!fp){ perror(fname); exit(1); }
    c->x_file[0] = c->y_file[0] = c->prefix_output[0] = '\0';
    c->N = 0; c->a = 3.0;

    char line[4096];
    while (fgets(line, sizeof(line), fp)){
        rstrip(line);
        if (!line[0] || line[0]=='#' || line[0]==';') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line, *val = eq + 1; lstrip(&val);

        if      (strcmp(key,"x_file")==0)         snprintf(c->x_file, sizeof(c->x_file), "%s", val);
        else if (strcmp(key,"y_file")==0)         snprintf(c->y_file, sizeof(c->y_file), "%s", val);
        else if (strcmp(key,"prefix_output")==0)  snprintf(c->prefix_output, sizeof(c->prefix_output), "%s", val);
        else if (strcmp(key,"N")==0)              c->N = atoll(val);
        else if (strcmp(key,"a")==0)              c->a = atof(val);
    }
    fclose(fp);

    if (!c->x_file[0] || !c->y_file[0] || !c->prefix_output[0] || c->N <= 0){
        fprintf(stderr, "Config missing x_file, y_file, prefix_output, or N\n");
        exit(1);
    }
}

int main(int argc, char **argv){
    if (argc != 2){
        fprintf(stderr, "Usage: %s config.ini\n", argv[0]);
        return 1;
    }
    cfg_t cfg; parse_cfg(argv[1], &cfg);

    /* build output name and ensure directory exists */
    ensure_parent_from_prefix(cfg.prefix_output);
    char fout[2048];
    if (snprintf(fout, sizeof(fout), "%sN%lld_d.dat", cfg.prefix_output, cfg.N) >= (int)sizeof(fout)){
        fprintf(stderr, "Output path too long\n"); return 1;
    }

    FILE *px = fopen(cfg.x_file, "r"); if (!px){ perror(cfg.x_file); return 1; }
    FILE *py = fopen(cfg.y_file, "r"); if (!py){ perror(cfg.y_file); fclose(px); return 1; }
    FILE *pd = fopen(fout, "w");       if (!pd){ perror(fout); fclose(px); fclose(py); return 1; }

    /* stream line-by-line */
    double xv, yv; long long i = 0;
    while (i < cfg.N && fscanf(px, "%lf", &xv) == 1 && fscanf(py, "%lf", &yv) == 1){
        double dv = cfg.a * xv + yv;
        if (fprintf(pd, "%.17g\n", dv) < 0){ perror("write d"); break; }
        ++i;
    }
    if (i != cfg.N) fprintf(stderr, "Warning: processed %lld values (expected %lld)\n", i, cfg.N);

    fclose(px); fclose(py); fclose(pd);
    printf("Wrote: %s (%lld values)\n", fout, i);
    return 0;
}
