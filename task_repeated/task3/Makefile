.RECIPEPREFIX := >
CC      ?= gcc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -pedantic
LDLIBS  ?= -lm

ifeq ($(OS),Windows_NT)
  EXE := .exe
endif

PROGS := task3_1$(EXE) task3_2$(EXE)

.PHONY: all run clean
all: $(PROGS)

task3_1$(EXE): task3_1.c
> $(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

task3_2$(EXE): task3_2.c
> $(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: task3_2$(EXE)
> ./task3_2$(EXE) config.ini

clean:
> -rm -f task3_1 task3_2 task3_1.exe task3_2.exe
