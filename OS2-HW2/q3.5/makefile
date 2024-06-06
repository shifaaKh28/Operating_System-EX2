# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -g -fprofile-arcs -ftest-coverage

# Phony targets
.PHONY: all clean

# Default target to build all
all: mync ttt

# Rule to build the 'mync' executable from 'mync.c'
mync: mync.c
	$(CC) $(CFLAGS) -o mync mync.c

# Rule to build the 'ttt' executable from 'ttt.o'
ttt: ttt.o
	$(CC) $(CFLAGS) ttt.o -o ttt -lm

# Rule to build the 'ttt.o' object file from 'ttt.c'
ttt.o: ttt.c
	$(CC) $(CFLAGS) -c ttt.c -o ttt.o

# Clean target to remove object files, executables, and coverage files
clean:
	rm -f *.o ttt mync *.gcda *.gcno *.gcov
