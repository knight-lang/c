#ifndef KN_DECLS_H
#define KN_DECLS_H

#include <stddef.h> /* size_t */
#include <stdlib.h> /* exit, abort */
#include <stdio.h>  /* fprintf, stderr */

#define kn_pragma(x) _Pragma(#x)

#ifdef __clang__
# define KN_CLANG_DIAG_PUSH _Pragma("clang diagnostic push")
# define KN_CLANG_DIAG_POP _Pragma("clang diagnostic pop")
# define KN_CLANG_DIAG_IGNORE(diag) kn_pragma(clang diagnostic ignored diag)
# define KN_CLANG_IGNORE(msg, x) KN_CLANG_DIAG_PUSH KN_CLANG_DIAG_IGNORE(msg) x KN_CLANG_DIAG_POP
// # pragma clang diagnostic ignored "-Wcast-qual" // I use `const` to indicate the args aren't modified
#else
# define KN_CLANG_DIAG_PUSH
# define KN_CLANG_DIAG_POP
# define KN_CLANG_DIAG_IGNORE(diag)
# define KN_CLANG_IGNORE(msg, x) x
#endif
	
#ifdef _MSC_VER
# define KN_MSVC_SUPPRESS(x) kn_pragma(warning( suppress : x ))
#else
# define KN_MSVC_SUPPRESS(x)
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

#ifdef __clang__
# pragma clang diagnostic ignored "-Wnullability-extension"
// # pragma clang diagnostic ignored "-Wnullability-completeness"
# define KN_NONNULL _Nonnull
# define KN_NULLABLE _Nullable
#else
# define KN_NONNULL
# define KN_NULLABLE
#endif

// #if KN_HAS_ATTRIBUTE(nonnull)
// # define KN_ARGS_NONNULL KN_ATTRIBUTE(nonnull)
// # define KN_ARG_NONNULL(...) KN_ATTRIBUTE(nonnull(__VA_ARGS__))
// #else
// # define KN_ARGS_NONNULL
// # define KN_ARG_NONNULL(...)
// #endif

// #if KN_HAS_ATTRIBUTE(returns_nonnull)
// # define KN_RETURNS_NONNULL KN_ATTRIBUTE(returns_nonnull)
// #else
// # define KN_RETURNS_NONNULL
// #endif

#if KN_HAS_BUILTIN(__builtin_expect)
# define KN_EXPECT(x, y) (__builtin_expect(x, y))
#else
# define KN_EXPECT(x, y) (x)
#endif /* KN_HAS_BUILTIN(__builtin_expect) */

#define KN_LIKELY(x)   KN_EXPECT(!!(x), 1)
#define KN_UNLIKELY(x) KN_EXPECT(!!(x), 0)

#if defined(__has_c_attribute) && (!defined(__GNUC__) || defined(__clang__))
# if __has_c_attribute(fallthrough)
#  define KN_FALLTHROUGH [[fallthrough]]
# endif
#endif
#ifndef KN_FALLTHROUGH
# ifdef __GNUC__
#  define KN_FALLTHROUGH __attribute__ ((fallthrough));
# else
#  define KN_FALLTHROUGH
# endif
#endif /* KN_FALLTHROUGH */

#define KN_DEFAULT_UNREACHABLE KN_CLANG_IGNORE("-Wcovered-switch-default", default: KN_UNREACHABLE)

#if KN_HAS_ATTRIBUTE(cold)
# define KN_COLD KN_ATTRIBUTE(cold)
#else
# define KN_COLD
#endif /* KN_HAS_ATTRIBUTE(cold) */ 

#define KN_NORETURN _Noreturn

void *kn_memdup(const void *mem, size_t length);

#endif /* !KN_DECLS_H */
