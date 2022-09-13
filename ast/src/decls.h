#ifndef KN_DECLS_H
#define KN_DECLS_H

#include <stdint.h>  /* uint64_t, int64_t */
#include <stdbool.h> /* bool */

/*
 * The type that represents values within Knight.
 *
 * All the different types within knight are represented in this type (which
 * internally uses bit masks). It's intentionally an opaque object---to interact
 * with it, you should use the relevant functions.
 *
 * To duplicate a value, use the `kn_value_clone` function---this returns a new
 * value which must be freed separately from the given one. To free a value,
 * pass it to `kn_value_free`.
 */
typedef uint64_t kn_value;

/*
 * The number type within Knight.
 *
 * Technically, this implementation only supports `int63_t` (as the extra bit
 * is used to indicate whether a `kn_value`'s a number or something else).
 */
typedef int64_t kn_number;

/*
 * The boolean type within Knight.
 *
 * This simply exists for completeness and functions identically to a `bool`.
 */
typedef bool kn_boolean;

// Forward declarations.
struct kn_ast;
struct kn_string;
struct kn_variable;
struct kn_list;

#ifdef KN_CUSTOM
struct kn_custom;
#endif /* KN_CUSTOM */

#endif /* !KN_DECLS_H */
