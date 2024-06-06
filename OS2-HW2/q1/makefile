# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -g -fprofile-arcs -ftest-coverage

# Phony targets
.PHONY: all clean

# Default target to build all
all: ttt

# Rule to build the executable 'ttt' from object file 'ttt.o'
ttt: ttt.o
	$(CC) $(CFLAGS) ttt.o -o ttt -lm

# Rule to build the object file 'ttt.o' from source file 'ttt.c'
ttt.o:
	$(CC) $(CFLAGS) -c ttt.c -o ttt.o

# Clean target to remove object files, executables, and coverage files
clean:
	rm -f *.o ttt *.gcno *.gcda *.gcov
