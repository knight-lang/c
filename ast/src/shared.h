#ifndef KN_SHARED_H
#define KN_SHARED_H

#include <stddef.h> /* size_t */
#include <stdlib.h> /* exit, abort */
#include <stdio.h>  /* fprintf, stderr */

#ifdef __has_builtin
# define KN_HAS_BUILTIN(x) __has_builtin(x)
#else
# define KN_HAS_BUILTIN(x) 0
#endif /* KN_HAS_BUILTIN */

#ifdef __has_attribute
# define KN_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
# define KN_HAS_ATTRIBUTE(x) 0
#endif /* KN_HAS_ATTRIBUTE */

#ifdef _MSC_VER
# define KN_ATTRIBUTE(attr) __declspec(x)
#elif defined(__GNUC__) || defined(__llvm__)
# define KN_ATTRIBUTE(x) __attribute__((x))
#else
# define KN_ATTRIBUTE(x)
#endif /* KN_ATTRIBUTE */

#if KN_HAS_BUILTIN(__builtin_expect)
# define KN_EXPECT(x, y) (__builtin_expect(x, y))
#else
# define KN_EXPECT(x, y) (x)
#endif /* KN_HAS_BUILTIN(__builtin_expect) */

#define KN_LIKELY(x)   KN_EXPECT(!!(x), 1)
#define KN_UNLIKELY(x) KN_EXPECT(!!(x), 0)

#if defined(__has_c_attribute) && __has_c_attribute(fallthrough)
# define KN_FALLTHROUGH [[fallthrough]]
#else
# define KN_FALLTHROUGH
#endif /* __has_c_attribute(fallthrough) */

#if KN_HAS_ATTRIBUTE(cold)
# define KN_COLD KN_ATTRIBUTE(cold)
#else
# define KN_COLD
#endif /* KN_HAS_ATTRIBUTE(cold) */ 

#ifdef NDEBUG
# if KN_HAS_BUILTIN(__builtin_unreachable)
#  define KN_UNREACHABLE do { __builtin_unreachable(); } while(0);
# else
#  define KN_UNREACHABLE do { (void) KN_UNLIKELY(1); abort(); } while(0);
# endif /* KN_HAS_BUILTIN(__builtin_unreachable) */
#else
# define KN_UNREACHABLE do { kn_die("bug at %s:%d", __FILE__, __LINE__); } while(0);
#endif /* NDEBUG */

#ifdef KN_RECKLESS
# define kn_error(...) KN_UNREACHABLE
#else
# define kn_error(...) kn_die(__VA_ARGS__)
#endif /* !KN_RECKLESS */

/**
 * A macros that's used to halt the execution of the program, writing the
 * given message to stderr before exiting with code 1.
 **/
#ifdef KN_FUZZING
# include <setjmp.h>  /* jmp_buf, setjmp, longjmp */
extern jmp_buf kn_play_start;
# define kn_die(...) longjmp(kn_play_start, 1)
#else
# define kn_die(...) (              \
   (void) KN_UNLIKELY(1),        \
   fprintf(stderr, __VA_ARGS__), \
   fputc('\n', stderr),          \
   exit(1))
#endif /* KN_FUZZING */

typedef unsigned long long kn_hash_t;

/**
 * Returns a hash for the first `length` characters of `str`.
 *
 * `str` must be at least `length` characters long, excluding any trailing `\0`
 **/
kn_hash_t kn_hash(const char *str, size_t length);

/**
 * Computes the hash of the first `length` characters of `str`, with the given
 * starting `hash`.
 *
 * This is useful to compute  hashes of non-sequential strings.
 **/
kn_hash_t kn_hash_acc(const char *str, size_t length, kn_hash_t hash);

/**
 * Allocates `size` bytes of memory and returns a pointer to them.
 *
 * This is identical to the stdlib's `malloc`, except the program is aborted
 * instead of returning `NULL`.
 **/
void *
#if KN_HAS_ATTRIBUTE(malloc)
KN_ATTRIBUTE(malloc)
#endif
#if KN_HAS_ATTRIBUTE(returns_nonnull)
KN_ATTRIBUTE(returns_nonnull)
#endif
xmalloc(size_t size);

/**
 * Resizes the pointer to a segment of at least `size` bytes of memory and
 * returns the new segment's pointer.
 *
 * This is identical to the stdlib's `realloc`, except the program is aborted
 * instead of returning `NULL`.
 **/
void *
#if KN_HAS_ATTRIBUTE(returns_nonnull)
KN_ATTRIBUTE(returns_nonnull)
#endif
xrealloc(void *ptr, size_t size);

#endif /* !KN_SHARED_H */
