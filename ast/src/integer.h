#ifndef KN_INTEGER_H
#define KN_INTEGER_H

#include <stdint.h>  /* uint64_t, int64_t */
#include <inttypes.h> /* PRId64 */
#include "shared.h" /* die */
#include <math.h> /* powl */

/**
 * The integer type within Knight.
 *
 * Technically, this implementation only supports `int63_t` (as the extra bit
 * is used to indicate whether a `kn_value`'s an integer or something else).
 **/
typedef int64_t kn_integer;

#define PRIdkn PRId64

struct kn_string;
struct kn_list;

static inline _Bool kn_integer_to_boolean(kn_integer integer) {
	return integer != 0;
}

struct kn_string *kn_integer_to_string(kn_integer integer);
struct kn_list *kn_integer_to_list(kn_integer integer);


#endif /* !KN_INTEGER_H */
