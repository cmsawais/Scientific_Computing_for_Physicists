.RECIPEPREFIX := >
CC      ?= gcc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -pedantic
LDLIBS  ?= -lm

# ---- Enable HDF5 with: make USE_HDF5=1 ----
ifeq ($(USE_HDF5),1)
  # Try pkg-config first; you can override from the command line if empty.
  HDF5_CFLAGS ?= $(shell pkg-config --cflags hdf5 2>/dev/null)
  HDF5_LDLIBS ?= $(shell pkg-config --libs   hdf5 2>/dev/null)
  CFLAGS += -DUSE_HDF5 $(HDF5_CFLAGS)
  LDLIBS += $(HDF5_LDLIBS)
endif

ifeq ($(OS),Windows_NT)
  EXE := .exe
endif

PROGS := task3_1b$(EXE) task3_2b$(EXE)

.PHONY: all runb clean
all: $(PROGS)

task3_1b$(EXE): task3_1b.c
> $(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

task3_2b$(EXE): task3_2b.c
> $(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

runb: task3_2b$(EXE)
> ./task3_2b$(EXE) config.ini

clean:
> -rm -f task3_1b task3_2b task3_1b.exe task3_2b.exe
