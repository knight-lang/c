#ifndef KN_NUMBER_H
#define KN_NUMBER_H

#include "decls.h"
#include <stdint.h>  /* uint64_t, int64_t */
#include <inttypes.h> /* PRId64 */
#include "shared.h" /* die */
#include <math.h> /* powl */

#define KN_CHECKED_OVERFLOWS

typedef uint64_t kn_unumber;
#define PRIdkn PRId64

static inline kn_boolean kn_number_to_boolean(kn_number number) {
	return number != 0;
}

struct kn_string *kn_number_to_string(kn_number number);
struct kn_list *kn_number_to_list(kn_number number);


#endif /* !KN_NUMBER_H */
