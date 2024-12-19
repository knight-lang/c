#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

void
#if KN_HAS_ATTRIBUTE(noreturn)
	KN_ATTRIBUTE(noreturn) // `_Noreturn` isn't allowed on externs.
#endif
(*kn_die_fn)(void) = abort;

void kn_die_impl(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);

	kn_die_fn();

// can't use `KN_UNREACHABLE` because it might call `kn_die` on debug builds
#if KN_HAS_BUILTIN(__builtin_unreachable)
	__builtin_unreachable();
	// __builtin_debugtrap <-- todo this too?
#else
	(void) KN_UNLIKELY(1);
	abort();
#endif /* KN_HAS_BUILTIN(__builtin_unreachable) */

}
