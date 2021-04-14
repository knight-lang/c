#ifndef KN_EXT_LIST_H
#define KN_EXT_LIST_H

#include "../src/value.h"
#include "../src/custom.h"
#include <stdbool.h>
#include <stdint.h>

extern const struct kn_custom_vtable kn_list_vtable;
extern struct kn_list kn_list_empty;

struct kn_list {
	size_t length;
	kn_value elements[];
};

struct kn_list *kn_list_alloc(size_t length);
void kn_list_free(struct kn_list *list);
void kn_list_dump(const struct kn_list *list);

kn_value kn_fn_extension_parse(const char **stream);

#endif /* !KN_EXT_LIST_H */

