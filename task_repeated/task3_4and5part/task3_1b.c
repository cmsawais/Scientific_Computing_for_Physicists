// task3_1.c  (adds HDF5 support via --format h5)
// Build HDF5 variant with:  make USE_HDF5=1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static long long parse_ll(const char *s) {
    char *end = NULL; errno = 0;
    long long v = strtoll(s, &end, 10);
    if (errno || end == s || *end != '\0') { fprintf(stderr,"Invalid N: '%s'\n", s); exit(1); }
    return v;
}

static int mkdir_p(const char *dir){
    if (!dir || !*dir) return 0;
    char tmp[2048]; snprintf(tmp,sizeof(tmp),"%s",dir);
    size_t len=strlen(tmp);
    while (len && (tmp[len-1]=='/'||tmp[len-1]=='\\')) tmp[--len]='\0';
    if (!len) return 0;
    char *p=tmp;
#ifdef _WIN32
    if (len>=2 && tmp[1]==':') p=tmp+2;
#endif
    if (*p=='/'||*p=='\\') ++p;
    for (; *p; ++p){
        if (*p=='/'||*p=='\\'){ char c=*p; *p='\0'; if (MKDIR(tmp)!=0 && errno!=EEXIST){perror(tmp);return -1;} *p=c; }
    }
    if (MKDIR(tmp)!=0 && errno!=EEXIST){ perror(tmp); return -1; }
    return 0;
}

static void ensure_dir_from_prefix(const char *prefix){
    char path[2048]; snprintf(path,sizeof(path),"%s",prefix);
    size_t len=strlen(path);
    while (len && (path[len-1]=='/'||path[len-1]=='\\')) path[--len]='\0';
    char *s1=strrchr(path,'/'), *s2=strrchr(path,'\\');
    char *sep = (s2 && s2>s1)? s2: s1;
    if (!sep) return;
    *sep = '\0';
    if (mkdir_p(path)!=0){ fprintf(stderr,"Failed to create %s\n", path); exit(1); }
}

#ifdef USE_HDF5
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
    if (argc < 3){
        fprintf(stderr,"Usage: %s N filename_prefix [--format text|h5]\n", argv[0]);
        return 1;
    }
    long long N = parse_ll(argv[1]);
    const char *prefix = argv[2];
    const char *format = "text";
    if (argc >= 5 && strcmp(argv[3],"--format")==0) format = argv[4];
    else if (argc == 4 && strncmp(argv[3], "--format=", 9)==0) format = argv[3]+9;

    ensure_dir_from_prefix(prefix);

    char fx[2048], fy[2048];
    if (strcmp(format,"h5")==0){
#ifndef USE_HDF5
        fprintf(stderr,"Rebuild with USE_HDF5=1 to enable HDF5.\n");
        return 1;
#endif
        snprintf(fx,sizeof(fx),"%sN%lld_x.h5", prefix, N);
        snprintf(fy,sizeof(fy),"%sN%lld_y.h5", prefix, N);

        double *x = (double*)malloc((size_t)N*sizeof(double));
        double *y = (double*)malloc((size_t)N*sizeof(double));
        if(!x||!y){ fprintf(stderr,"malloc failed\n"); return 1; }
        for (long long i=0;i<N;++i){ x[i]=0.1; y[i]=7.1; }

#ifdef USE_HDF5
        write_vector_h5(fx, x, N);
        write_vector_h5(fy, y, N);
#endif
        free(x); free(y);
    } else {
        snprintf(fx,sizeof(fx),"%sN%lld_x.dat", prefix, N);
        snprintf(fy,sizeof(fy),"%sN%lld_y.dat", prefix, N);
        FILE *fpx=fopen(fx,"w"); if(!fpx){perror(fx); return 1;}
        FILE *fpy=fopen(fy,"w"); if(!fpy){perror(fy); fclose(fpx); return 1;}
        static char bufX[1<<16], bufY[1<<16];
        setvbuf(fpx, bufX, _IOFBF, sizeof(bufX));
        setvbuf(fpy, bufY, _IOFBF, sizeof(bufY));
        for (long long i=0;i<N;++i){
            if (fprintf(fpx,"%.17g\n", 0.1) < 0){ perror("write x"); break; }
            if (fprintf(fpy,"%.17g\n", 7.1) < 0){ perror("write y"); break; }
        }
        fclose(fpx); fclose(fpy);
    }
    printf("Wrote:\n  %s\n  %s\n", fx, fy);
    return 0;
}
