#ifndef KN_INTEGER_H
#define KN_INTEGER_H

#include "decls.h"
#include <stdint.h>  /* uint64_t, int64_t */
#include <inttypes.h> /* PRId64 */
#include "shared.h" /* die */
#include <math.h> /* powl */

#define KN_CHECKED_OVERFLOWS

typedef uint64_t kn_uinteger;
#define PRIdkn PRId64

static inline kn_boolean kn_integer_to_boolean(kn_integer integer) {
	return integer != 0;
}

struct kn_string *kn_integer_to_string(kn_integer integer);
struct kn_list *kn_integer_to_list(kn_integer integer);


#endif /* !KN_INTEGER_H */
