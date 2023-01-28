#ifndef ERROR_H
#define ERROR_H

#include <stdlib.h>
#include <stdio.h>

#define fatalf(...) do { \
	fprintf(stderr, __VA_ARGS__); \
	exit(EXIT_FAILURE); \
} while(0)

#endif