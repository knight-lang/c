#ifndef KN_CUSTOM
// declare _something_ so we don't have an empty compilation unit when not
// compiling with custom structs.
struct kn_dummy_struct_;
#endif /* !KN_CUSTOM */

#ifdef KN_CUSTOM
#include <assert.h> /* assert */
#include "custom.h" /* prototypes, kn_custom, kn_custom_vtable, kn_value,
                       kn_integer, kn_boolean, kn_string, size_t */
#include "shared.h" /* kn_heap_malloc */
#include <stdlib.h> /* free, NULL */

struct kn_custom *kn_custom_alloc(
	size_t size,
	const struct kn_custom_vtable *vtable
) {
	assert(vtable != NULL);

	struct kn_custom *custom = kn_heap_malloc(size + sizeof(struct kn_custom));

#ifdef KN_USE_REFCOUNT
	kn_refcount(custom) = 1;
#endif /* KN_USE_REFCOUNT */
	custom->vtable = vtable;

	return custom;
}

void kn_custom_dealloc(struct kn_custom *custom) {
	if (custom->vtable->free != NULL)
		custom->vtable->free((void *) custom->data);

	kn_heap_free(custom);
}

#endif /* KN_CUSTOM */
