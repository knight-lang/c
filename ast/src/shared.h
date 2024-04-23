#ifndef KN_SHARED_H
#define KN_SHARED_H

#include <stddef.h> /* size_t */
#include <stdlib.h> /* exit, abort */
#include <stdio.h>  /* fprintf, stderr */

#ifdef __clang__
# define kn_macro_to_str(x) #x
# define kn_clang_pragma(x) _Pragma(#x)
# define kn_clang_ignore(msg, x) _Pragma("clang diagnostic push") \
	kn_clang_pragma(clang diagnostic ignored kn_macro_to_str("-W" msg)) \
	x \
	_Pragma("clang diagnostic pop")
#else
# define kn_clang_ignore(msg, x) x
#endif

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
# define KN_ATTRIBUTE(x) __declspec(x)
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

#if defined(__has_c_attribute) && __has_c_attribute(fallthrough) && (!defined(__GNUC__) || defined(__clang__))
# define KN_FALLTHROUGH [[fallthrough]]
#else
# define KN_FALLTHROUGH /* fallthrough */
#endif /* __has_c_attribute(fallthrough) */

#if KN_HAS_ATTRIBUTE(cold)
# define KN_COLD KN_ATTRIBUTE(cold)
#else
# define KN_COLD
#endif /* KN_HAS_ATTRIBUTE(cold) */ 

#ifdef NDEBUG
# if KN_HAS_BUILTIN(__builtin_unreachable)
#  define KN_UNREACHABLE do { __builtin_unreachable(); } while(0);
	// __builtin_debugtrap <-- todo this too?
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
#endif /* !KN_SHARED_H */
