#ifndef KN_EXT_UFUNC_H
#define KN_EXT_UFUNC_H

#include "../src/value.h"
#include "../src/custom.h"
#include <stdbool.h>
#include <stdint.h>

extern const struct kn_custom_vtable ufunc_vtable;

#define MAXARGC 255

// user function.
struct ufunc {
	unsigned char paramc;
	kn_value body;
	struct kn_variable *params[];
};

void free_ufunc(struct ufunc *);

kn_value parse_extension_ufunc(void);

#endif /* !KN_EXT_UFUNC_H */
