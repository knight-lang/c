#ifdef KN_CUSTOM
#include "custom.h"
#include <assert.h>
#include "shared.h"

struct kn_custom *kn_custom_alloc(
	size_t size,
	const struct kn_custom_vtable *vtable
) {
	assert(vtable != NULL);

	struct kn_custom *custom = xmalloc(size + sizeof(struct kn_custom));

	custom->refcount = 1;
	custom->vtable = vtable;

	return custom;
}

void kn_custom_free(struct kn_custom *custom) {
	if (--custom->refcount != 0)
		return;

	if (custom->vtable->free != NULL)
		custom->vtable->free((void *) custom->data);

	free(custom);
}

struct kn_custom *kn_custom_clone(struct kn_custom *custom) {
	++custom->refcount;

	return custom;
}

#endif /* KN_CUSTOM */
