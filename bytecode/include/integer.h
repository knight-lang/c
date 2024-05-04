#ifndef KN_INTEGER_H
#define KN_INTEGER_H

#include <inttypes.h>

/**
 * The integer type within Knight.
 *
 * Technically, this implementation only supports `int63_t` (as the extra bit
 * is used to indicate whether a `kn_value`'s an integer or something else).
 **/
typedef intptr_t kn_integer;

/**
 * The format conversion specifier for `kn_integer`.
 **/
#define PRIdkn PRIdPTR

// Forward declarations.
struct kn_string;
struct kn_list;

/**
 * Returns a string representation of `integer`.
 * 
 * For efficiency purposes, this doesn't actually allocate the string (and thus you also don't need
 * to free it). To get an allocated version, use `kn_string_clone_static` on it.
 **/
struct kn_string *kn_integer_to_string(kn_integer integer);

/**
 * Returns a list representation of `integer`.
 * 
 * For efficiency purposes, this doesn't actually allocate the list (and thus you also don't need
 * to free it). To get an allocated version, use `kn_list_clone_integer` on it.
 **/
struct kn_list *kn_integer_to_list(kn_integer integer);

#endif /* !KN_INTEGER_H */
