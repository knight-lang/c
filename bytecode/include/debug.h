#ifndef KN_DEBUG_H
#define KN_DEBUG_H

#include "decls.h"

#ifdef KN_LOG
# define kn_logn(...) printf(__VA_ARGS__), fflush(stdout)
# define kn_log(...) printf(__VA_ARGS__), putc('\n', stdout), fflush(stdout)
#else
# define kn_logn(...)
# define kn_log(...)
#endif /* !KN_LOG */

/**
 * A macros that's used to halt the execution of the program, writing the
 * given message to stderr before exiting with code 1.
 **/
#define kn_die(...) ((void) KN_UNLIKELY(1), kn_die_impl(__VA_ARGS__))
void KN_NORETURN kn_die_impl(const char *fmt, ...);

extern void
#if KN_HAS_ATTRIBUTE(noreturn)
	KN_ATTRIBUTE(noreturn) // `_Noreturn` isn't allowed on externs.
#endif
(*kn_die_fn)(void);


#ifdef KN_RECKLESS
# define kn_bugm(...) KN_UNREACHABLE
#else
# define kn_bugm(fmt, ...) kn_die("BUG at %s:%d (%s): " fmt, \
                                  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#endif /* KN_RECKLESS */
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
# define kn_assertm(cond, ...) do { (void) 0; } while (0)
#endif

#define kn_assert(cond) kn_assertm(cond, "assertion `%s` failed", #cond)

#define kn_assert_eq(lhs, rhs) kn_assertm((lhs) == (rhs), \
	"equality failed: %lld != %lld, but should be", (long long) (lhs), (long long) rhs)

#define kn_assert_ne(lhs, rhs) kn_assertm((lhs) != (rhs), \
	"equality failed: %lld == %lld, but shouldnt be", (long long) (lhs), (long long) rhs)

#define kn_assert_nonnull(ptr) kn_assert_ne(ptr, NULL)
#endif
