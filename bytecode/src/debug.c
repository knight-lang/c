#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

void kn_die_fn(void (*fn)(void), const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fn();
	abort();
}
