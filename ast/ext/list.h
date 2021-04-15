#ifndef KN_EXT_LIST_H
#define KN_EXT_LIST_H

#include "../src/value.h"
#include "../src/custom.h"
#include <stdbool.h>
#include <stdint.h>

extern const struct kn_custom_vtable kn_list_vtable;

struct kn_list {
	size_t length;
	bool idempotent;
	kn_value *elements;
};

kn_value kn_parse_extension_list(void);

#endif /* !KN_EXT_LIST_H */

