#ifndef KN_SHARED_H
#define KN_SHARED_H

#include <stddef.h> /* size_t */
#include <stdlib.h> /* exit, abort */
#include <stdio.h>  /* fprintf, stderr */

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif /* !__has_builtin */

#if KN_USE_EXTENSIONS
# define KN_ATTRIBUTE(x) __attribute__(x)
#else
# define KN_ATTRIBUTE(x)
#endif /* KN_USE_EXTENSIONS */

#if __has_builtin(__builtin_expect)
# define KN_EXPECT(x, y) (__builtin_expect(x, y))
#else
# define KN_EXPECT(x, y) (x)
#endif /* __has_builtin(__builtin_expect) */

#if defined(__has_c_attribute) && __has_c_attribute(fallthrough)
# define KN_FALLTHROUGH [[fallthrough]]
#else
# define KN_FALLTHROUGH
#endif /* __has_c_attribute(fallthrough) */

#ifdef NDEBUG
# if __has_builtin(__builtin_unreachable)
#  define KN_UNREACHABLE() do { (void) KN_UNLIKELY(1); __builtin_unreachable(); } while(0)
# else
#  define KN_UNREACHABLE() do { (void) KN_UNLIKELY(1); abort(); } while(0)
# endif /* __has_builtin(__builtin_unreachable) */
#else
# define KN_UNREACHABLE() do { die("bug at %s:%d", __FILE__, __LINE__); } while(0)
#endif /* NDEBUG */

#define KN_LIKELY(x) KN_EXPECT(!!(x), 1)
#define KN_UNLIKELY(x) KN_EXPECT(!!(x), 0)

#ifndef KN_RECKLESS
#define kn_error(...) die(__VA_ARGS__)
#else
#define kn_error(...) KN_UNREACHABLE()
#endif /* !KN_RECKLESS */

/*
 * A macros that's used to halt the execution of the program, writing the
 * given message to stderr before exiting with code 1.
 */
#define die(...) ((void) KN_UNLIKELY(1), fprintf(stderr, __VA_ARGS__), fputc('\n', stderr), exit(1))

/*
 * Returns a hash for the first `length` characters of `str`.
 *
 * `str` must be at least `length` characters long, excluding any trailing `\0`
 */
unsigned long kn_hash(const char *str, size_t length);

/*
 * Computes the hash of the first `length` characters of `str`, with the given
 * starting `hash`.
 *
 * This is useful to compute  hashes of non-sequential strings.
 */
unsigned long kn_hash_acc(const char *str, size_t length, unsigned long hash);

/*
 * Allocates `size` bytes of memory and returns a pointer to them.
 *
 * This is identical to the stdlib's `malloc`, except the program is aborted
 * instead of returning `NULL`.
 */
void *xmalloc(size_t size) KN_ATTRIBUTE((malloc));

/*
 * Resizes the pointer to a segment of at least `size` bytes of memory and
 * returns the new segment's pointer.
 *
 * This is identical to the stdlib's `realloc`, except the program is aborted
 * instead of returning `NULL`.
 */
void *xrealloc(void *ptr, size_t size);

extern unsigned kn_indentation;
void kn_indent(FILE *out);

#endif /* !KN_SHARED_H */
