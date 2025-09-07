// 1.c
// Usage: ./1 config.ini
// Requires GSL; HDF5 is optional (enable with -DUSE_HDF5)
//
// config.ini keys:
//   x_file=./out/vector_N10_x.(dat|h5)
//   y_file=./out/vector_N10_y.(dat|h5)
//   N=10
//   a=3.0
//   prefix_output=./out/vector_
//   format=text   # or h5

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

#ifdef USE_HDF5
  #include <hdf5.h>
#endif

#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>   // for gsl_vector_memcpy (declared in gsl_vector.h) and BLAS helpers

typedef struct {
    char x_file[1024];
    char y_file[1024];
    char prefix_output[1024];
    char format[16];      // "text" or "h5"
    long long N;
    double a;
} cfg_t;

/* ------------ small utils ------------ */
static void rstrip(char *s){ size_t n=strlen(s); while(n && (s[n-1]=='\n'||s[n-1]=='\r'||isspace((unsigned char)s[n-1]))) s[--n]=0; }
static void lstrip(char **p){ while(**p && isspace((unsigned char)**p)) (*p)++; }

static int mkdir_p(const char *dir){
    if (!dir||!*dir) return 0;
    char tmp[2048]; snprintf(tmp,sizeof(tmp),"%s",dir);
    size_t len=strlen(tmp); while(len && (tmp[len-1]=='/'||tmp[len-1]=='\\')) tmp[--len]=0;
    if (!len) return 0;
    char *q=tmp;
#ifdef _WIN32
    if (len>=2 && tmp[1]==':') q=tmp+2;
#endif
    if (*q=='/'||*q=='\\') ++q;
    for (; *q; ++q){
        if (*q=='/'||*q=='\\'){ char c=*q; *q=0; if (MKDIR(tmp)!=0 && errno!=EEXIST){ perror(tmp); return -1; } *q=c; }
    }
    if (MKDIR(tmp)!=0 && errno!=EEXIST){ perror(tmp); return -1; }
    return 0;
}

static void ensure_dir_from_prefix(const char *prefix){
    char path[2048]; snprintf(path,sizeof(path),"%s",prefix);
    size_t len=strlen(path); while(len && (path[len-1]=='/'||path[len-1]=='\\')) path[--len]=0;
    char *s1=strrchr(path,'/'), *s2=strrchr(path,'\\'); char *sep=(s2&&s2>s1)?s2:s1;
    if (!sep) return; *sep=0;
    if (mkdir_p(path)!=0){ fprintf(stderr,"Failed to create %s\n", path); exit(1); }
}

/* ------------ config ------------ */
static void parse_cfg(const char *fname, cfg_t *c){
    FILE *fp=fopen(fname,"r"); if(!fp){ perror(fname); exit(1); }
    c->x_file[0]=c->y_file[0]=c->prefix_output[0]=c->format[0]=0; c->N=0; c->a=3.0;
    char line[4096];
    while (fgets(line,sizeof(line),fp)){
        rstrip(line); if(!line[0]||line[0]=='#'||line[0]==';') continue;
        char *eq=strchr(line,'='); if(!eq) continue; *eq=0;
        char *key=line, *val=eq+1; lstrip(&val);
        if      (strcmp(key,"x_file")==0)         snprintf(c->x_file,sizeof(c->x_file),"%s",val);
        else if (strcmp(key,"y_file")==0)         snprintf(c->y_file,sizeof(c->y_file),"%s",val);
        else if (strcmp(key,"prefix_output")==0)  snprintf(c->prefix_output,sizeof(c->prefix_output),"%s",val);
        else if (strcmp(key,"format")==0)         snprintf(c->format,sizeof(c->format),"%s",val);
        else if (strcmp(key,"N")==0)              c->N = atoll(val);
        else if (strcmp(key,"a")==0)              c->a = atof(val);
    }
    fclose(fp);
    if (!c->x_file[0]||!c->y_file[0]||!c->prefix_output[0]||c->N<=0){
        fprintf(stderr,"Config missing x_file, y_file, prefix_output, or N\n"); exit(1);
    }
    if (!c->format[0]) snprintf(c->format,sizeof(c->format),"%s","text");
}

/* ------------ text I/O ------------ */
static void read_vector_text(const char *fname, double *v, long long N){
    FILE *fp=fopen(fname,"r"); if(!fp){ perror(fname); exit(1); }
    for (long long i=0;i<N;++i){ if (fscanf(fp,"%lf",&v[i])!=1){ fprintf(stderr,"Read fail at %lld in %s\n", i, fname); exit(1);} }
    fclose(fp);
}
static void write_vector_text(const char *fname, const double *v, long long N){
    FILE *fp=fopen(fname,"w"); if(!fp){ perror(fname); exit(1); }
    for (long long i=0;i<N;++i) fprintf(fp,"%.17g\n", v[i]);
    fclose(fp);
}

/* ------------ HDF5 I/O (optional) ------------ */
#ifdef USE_HDF5
static void read_vector_h5(const char *fname, double *v, long long N){
    hid_t f = H5Fopen(fname, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (f < 0){ fprintf(stderr,"H5Fopen failed: %s\n", fname); exit(1); }
    hid_t ds = H5Dopen2(f, "data", H5P_DEFAULT);
    if (ds < 0){ fprintf(stderr,"H5Dopen2 failed: dataset 'data'\n"); exit(1); }
    hid_t sp = H5Dget_space(ds);
    hsize_t dims[1]; if (H5Sget_simple_extent_ndims(sp)!=1 || H5Sget_simple_extent_dims(sp, dims, NULL)!=1 || dims[0] != (hsize_t)N){
        fprintf(stderr,"Dataset size mismatch in %s (got %llu, expected %lld)\n", fname, (unsigned long long)dims[0], N); exit(1);
    }
    if (H5Dread(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, v) < 0){ fprintf(stderr,"H5Dread failed\n"); exit(1); }
    H5Sclose(sp); H5Dclose(ds); H5Fclose(f);
}
static void write_vector_h5(const char *fname, const double *v, long long N){
    hid_t f = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (f < 0){ fprintf(stderr,"H5Fcreate failed: %s\n", fname); exit(1); }
    hsize_t dims[1] = { (hsize_t)N };
    hid_t sp = H5Screate_simple(1, dims, NULL);
    hid_t ds = H5Dcreate2(f, "data", H5T_NATIVE_DOUBLE, sp, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (ds < 0){ fprintf(stderr,"H5Dcreate2 failed\n"); exit(1); }
    if (H5Dwrite(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, v) < 0){ fprintf(stderr,"H5Dwrite failed\n"); exit(1); }
    H5Dclose(ds); H5Sclose(sp); H5Fclose(f);
}
#endif

int main(int argc, char **argv){
    if (argc != 2){ fprintf(stderr,"Usage: %s config.ini\n", argv[0]); return 1; }
    cfg_t cfg; parse_cfg(argv[1], &cfg);

    ensure_dir_from_prefix(cfg.prefix_output);
    char fout[2048];
    int ok = snprintf(fout,sizeof(fout),
        (strcmp(cfg.format,"h5")==0) ? "%sN%lld_d.h5" : "%sN%lld_d.dat",
        cfg.prefix_output, cfg.N);
    if (ok < 0 || ok >= (int)sizeof(fout)){ fprintf(stderr,"Output path too long\n"); return 1; }

    /* allocate and load */
    double *x = (double*)malloc((size_t)cfg.N*sizeof(double));
    double *y = (double*)malloc((size_t)cfg.N*sizeof(double));
    double *d = (double*)malloc((size_t)cfg.N*sizeof(double));
    if(!x||!y||!d){ fprintf(stderr,"malloc failed\n"); return 1; }

    if (strcmp(cfg.format,"h5")==0){
#ifndef USE_HDF5
        fprintf(stderr,"HDF5 requested but program not built with USE_HDF5\n");
        return 1;
#else
        read_vector_h5(cfg.x_file, x, cfg.N);
        read_vector_h5(cfg.y_file, y, cfg.N);
#endif
    } else {
        read_vector_text(cfg.x_file, x, cfg.N);
        read_vector_text(cfg.y_file, y, cfg.N);
    }

    /* ---- GSL: d = a*x + 1*y  ---- */
    gsl_vector_view vx = gsl_vector_view_array(x, (size_t)cfg.N);
    gsl_vector_view vy = gsl_vector_view_array(y, (size_t)cfg.N);
    gsl_vector_view vd = gsl_vector_view_array(d, (size_t)cfg.N);

    gsl_vector_memcpy(&vd.vector, &vy.vector);                  // d = y
    gsl_vector_axpby(cfg.a, &vx.vector, 1.0, &vd.vector);       // d = a*x + 1*d

    /* write output */
    if (strcmp(cfg.format,"h5")==0){
#ifdef USE_HDF5
        write_vector_h5(fout, d, cfg.N);
#endif
    } else {
        write_vector_text(fout, d, cfg.N);
    }

    free(x); free(y); free(d);
    printf("Wrote: %s (%lld values)\n", fout, cfg.N);
    return 0;
}
