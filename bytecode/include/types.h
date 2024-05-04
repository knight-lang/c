#ifndef KN_TYPES_H
#define KN_TYPES_H

/**
 * This file exists so that the header files of types dependent upon the typedef'd `kn_value`,
 * `kn_integer`, and `kn_boolean` can use the declaration internally.
 **/

#include "boolean.h"
#include "integer.h"
#include <stdint.h>

/**
 * The type that represents values within Knight.
 *
 * All the different types within knight are represented in this type (which internally uses bit
 * masks). It's intentionally an opaque object---to interact with it, you should use the relevant
 * functions.
 *
 * To duplicate a value, use the `kn_value_clone` function---this returns a new value which must be
 * freed separately from the given one. To free a value, pass it to `kn_value_free`.
 **/
typedef uintptr_t kn_value;

// Forward declarations.
struct kn_ast;
struct kn_string;
struct kn_variable;
struct kn_list;
struct kn_env;

#ifdef KN_CUSTOM
struct kn_custom;
#endif /* KN_CUSTOM */

#endif /* !KN_TYPES_H */
