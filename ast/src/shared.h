#ifndef KN_SHARED_H
#define KN_SHARED_H

#include <stddef.h> /* size_t */
#include <stdlib.h> /* exit, abort */
#include <stdio.h>  /* fprintf, stderr */

#ifdef KN_USE_EXTENSIONS
# define KN_ATTRIBUTE(x) __attribute__(x)
# define KN_EXPECT(x, y) (__builtin_expect(x, y))
# ifdef NDEBUG
#  define KN_UNREACHABLE() (__builtin_unreachable())
# else
#  define KN_UNREACHABLE() \
	die("Issue at  %s:%s:%d", __FILE__, __FUNCTION__, __LINE__)
# endif /* NDEBUG */
#else
# define KN_EXPECT(x, y) (x)
# define KN_ATTRIBUTE(x)
# ifdef NDEBUG
#  define KN_UNREACHABLE() (abort())
# else
#  define KN_UNREACHABLE() \
	die("Issue at  %s:%s:%d", __FILE__, __FUNCTION__, __LINE__)
# endif /* NDEBUG */
#endif /* KN_USE_EXTENSIONS */

#define KN_LIKELY(x) (__builtin_expect(!!(x), 1))
#define KN_UNLIKELY(x) (__builtin_expect(!!(x), 0))

/*
 * A macros that's used to halt the execution of the program, writing the
 * given message to stderr before exiting with code 1.
 */
#define die(...) (fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"), exit(1))

/*
 * Returns a hash for the first `length` characters of `str`.
 *
 * `str` must be at least `length` characters long, excluding any trailing `\0`
 */
unsigned long kn_hash(const char *str, size_t length);

/*
 * Allocates `size` bytes of memory and returns a pointer to them.
 *
 * This is identical to the stdlib's `malloc`, except the program is aborted
 * instead of returning `NULL`.
 *
 * The `size`, when converted to an `ssize_t`, must be nonnegative.
 */
void *xmalloc(size_t size) KN_ATTRIBUTE((malloc));

/*
 * Resizes the pointer to a segment of at least `size` bytes of memory and
 * returns the new segment's pointer.
 *
 * This is identical to the stdlib's `realloc`, except the program is aborted
 * instead of returning `NULL`.
 *
 * The `size`, when converted to an `ssize_t`, must be nonnegative.
 */
void *xrealloc(void *ptr, size_t size);

#endif /* !KN_SHARED_H */
