#ifndef KN_EXT_UFUNC_H
#define KN_EXT_UFUNC_H

#include "../src/value.h"
#include "../src/custom.h"
#include <stdbool.h>
#include <stdint.h>

extern const struct kn_custom_vtable kn_list_vtable;

#define MAXARGC 255
// user function.
struct kn_ufunc {
	unsigned char paramc;
	kn_value body;
	struct kn_variable *params[];
};

kn_value kn_parse_extension_ufunc(void);

#endif /* !KN_EXT_UFUNC_H */

