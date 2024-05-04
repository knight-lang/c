#ifndef KN_DEBUG_H
#define KN_DEBUG_H

#undef KN_DECLS_H
#include "decls.h"

/**
 * A macros that's used to halt the execution of the program, writing the
 * given message to stderr before exiting with code 1.
 **/
#define kn_die(...) ((void) KN_UNLIKELY(1), kn_die_fn(abort, __VA_ARGS__))
void KN_NORETURN kn_die_fn(void (*fn)(void), const char *fmt, ...);

#define kn_bugm(fmt, ...) kn_die("BUG at %s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__)
#define kn_bug(msg) kn_bugm("%s", msg)

#ifdef KN_NDEBUG
# if KN_HAS_BUILTIN(__builtin_unreachable)
#  define KN_UNREACHABLE do { __builtin_unreachable(); } while(0);
	// __builtin_debugtrap <-- todo this too?
# else
#  define KN_UNREACHABLE do { (void) KN_UNLIKELY(1); abort(); } while(0);
# endif /* KN_HAS_BUILTIN(__builtin_unreachable) */
#else
# define KN_UNREACHABLE do { kn_bug("unreachable encountered"); } while(0);
#endif /* NDEBUG */

#if defined(__STDC_VERSION__)
# if __STDC_VERSION__ >= 2011121L
#  define KN_STATIC_ASSERTM(cond, msg) _Static_assert(cond, msg)
# endif
#endif
#ifndef KN_STATIC_ASSERTM
# define KN_STATIC_ASSERTM(cond, msg) struct kn_static_assert_disabled
#endif
#define KN_STATIC_ASSERT(cond) KN_STATIC_ASSERTM(cond, "static assertion `" #cond " `failed.")


#ifndef KN_NDEBUG
# define kn_assertm(cond, ...) do { if (!(cond)) kn_bugm(__VA_ARGS__); } while(0)
#else
# define kn_assertm(cond, ...) do { (void) 0; } while(0)
#endif

#define kn_assert(cond) kn_assertm(cond, "assertion `" #cond "` failed.")

#define kn_assert_eq(lhs, rhs) kn_assertm((lhs) == (rhs), \
	"equality failed: %lld != %lld, but should bed", \
	(long long) (lhs), (long long) rhs)

#define kn_assert_ne(lhs, rhs) kn_assertm((lhs) != (rhs), \
	"equality failed: %lld == %lld, but shouldnt be", \
	(long long) (lhs), (long long) rhs)

#define kn_assert_nonnull(ptr) kn_assert_ne(ptr, NULL)
#endif
