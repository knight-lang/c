#include "list.h"
#include "shared.h"
#include <assert.h>

struct kn_list kn_list_empty = {
	.refcount = 1,
	.length = 0,
};


struct kn_list *kn_list_alloc(size_t capacity) {
	struct kn_list *list = xmalloc(sizeof(struct kn_list));

	list->refcount = 1;
	list->length = 0;
	list->elements = xmalloc(sizeof(kn_value) * capacity);

	return list;
}

struct kn_list *kn_list_clone(struct kn_list *list) {
	assert(list->refcount);

	list->refcount++;

	return list;
}

void kn_list_deallocate(struct kn_list *list) {
	assert(!list->refcount);

	for (unsigned i = 0; i < list->length; ++i)
		kn_value_free(list->elements[i]);

	free(list->elements);
	free(list);
}

bool kn_list_equal(const struct kn_list *lhs, const struct kn_list *rhs) {
	if (lhs->length != rhs->length)
		return false;

	for (unsigned i = 0; i < lhs->length; ++i)
		// if !kn_Value_e
		assert(false);

	return true;
}

struct kn_string *kn_list_join(const struct kn_list *list, const char *sep) {
	(void) list;
	(void) sep;
	die("todo: kn list join");
}
