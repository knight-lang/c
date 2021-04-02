#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>

#define DIE(...) do { printf(__VA_ARGS__); exit(1); } while (0)

#endif
