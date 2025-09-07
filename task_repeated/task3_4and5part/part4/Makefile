.RECIPEPREFIX := >
CC      ?= gcc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -pedantic
LDLIBS  ?= -lm

# ---- GSL (required for task3_2gsl) ----
GSL_CFLAGS ?= $(shell pkg-config --cflags gsl 2>/dev/null)
GSL_LDLIBS ?= $(shell pkg-config --libs   gsl 2>/dev/null)
CFLAGS     += $(GSL_CFLAGS)
LDLIBS     += $(GSL_LDLIBS)

# ---- HDF5 toggle (optional) ----
ifeq ($(USE_HDF5),1)
> HDF5_CFLAGS ?= $(shell pkg-config --cflags hdf5 2>/dev/null)
> HDF5_LDLIBS ?= $(shell pkg-config --libs   hdf5 2>/dev/null)
> CFLAGS      += -DUSE_HDF5 $(HDF5_CFLAGS)
> LDLIBS      += $(HDF5_LDLIBS)
endif

ifeq ($(OS),Windows_NT)
> EXE := .exe
endif

# adjust if your generator file name is different
PROGS := task3_1b$(EXE) task3_2gsl$(EXE)

.PHONY: all run_text run_h5 clean
all: $(PROGS)

task3_1b$(EXE): task3_1b.c
> $(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

task3_2gsl$(EXE): task3_2gsl.c
> $(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

# convenience runners (expects config.ini in cwd)
run_text: task3_2gsl$(EXE)
> ./task3_2gsl$(EXE) config.ini

run_h5: task3_2gsl$(EXE)
> ./task3_2gsl$(EXE) config.ini

clean:
> -rm -f task3_1b task3_2gsl task3_1b.exe task3_2gsl.exe
