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

#ifdef KN_CHECKED_OVERFLOWS
#define check_overflow(lhs, op, rhs) do { \
   } while(0)
   // if ((kn_unumber) lhs op (kn_unumber) rhs != (kn_unumber) lhs op rhs) \
   //    die("integer overflow for `" #op "`: %" PRIdkn " " #op " %" PRIdkn "overflows", lhs, rhs); \
   // } while (0)
#else
#define check_overflow(lhs, op, rhs) do {} while(0)
#endif /* KN_CHECKED_OVERFLOWS */

static inline kn_number kn_number_negate(kn_number number) {
#ifdef KN_CHECKED_OVERFLOWS
   // if (-(kn_unumber) number != -number)
   //    die("integer overflow for `~`: ~%" PRIdkn "overflows", number);
#endif /* KN_CHECKED_OVERFLOWS */
   return -number;
}

static inline kn_number kn_number_add(kn_number lhs, kn_number rhs) {
   check_overflow(lhs, +, rhs);
   return lhs + rhs;
}

static inline kn_number kn_number_subtract(kn_number lhs, kn_number rhs) {
   check_overflow(lhs, -, rhs);
   return lhs - rhs;
}

static inline kn_number kn_number_multiply(kn_number lhs, kn_number rhs) {
   check_overflow(lhs, *, rhs);
   return lhs * rhs;
}

static inline kn_number kn_number_divide(kn_number lhs, kn_number rhs) {
#ifndef KN_RECKLESS
   if (!lhs) die("division by zero");
#endif /* !KN_RECKLESS */

   check_overflow(lhs, /, rhs);
   return lhs / rhs;
}

static inline kn_number kn_number_remainder(kn_number lhs, kn_number rhs) {
#ifndef KN_RECKLESS
   if (!lhs) die("modulo by zero");
#endif /* !KN_RECKLESS */

   check_overflow(lhs, %, rhs);
   return lhs % rhs;
}

static inline kn_number kn_number_power(kn_number lhs, kn_number rhs) {
#ifndef KN_RECKLESS
   if (rhs < 0) die("exponent by negative power");
#endif /* !KN_RECKLESS */

   // this'll return valid numbers for anything within the range.
   return (kn_number) powl(lhs, rhs);
}

static inline kn_boolean kn_number_to_boolean(kn_number number) {
   return number != 0;
}

static inline void kn_number_dump(kn_number number, FILE *out) {
   fprintf(out, "Number(%" PRIdkn ")", number);
}

struct kn_string *kn_number_to_string(kn_number number);
struct kn_list *kn_number_to_list(kn_number number);


#endif /* !KN_NUMBER_H */
