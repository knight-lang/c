#include "list.h"
#include "shared.h"
#include "string.h"
#include <string.h>
#include <assert.h>

struct kn_list kn_list_empty = {
	.refcount = 1,
	.length = 0,
};


struct kn_list *kn_list_alloc(size_t capacity) {
	struct kn_list *list = xmalloc(sizeof(struct kn_list));

	list->refcount = 1;
	list->length = capacity;
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

kn_value kn_list_head(const struct kn_list *list) {
	if (!list->length) return KN_UNDEFINED;

	return kn_value_clone(list->elements[0]);
}

struct kn_list *kn_list_tail(const struct kn_list *list) {
	if (!list->length) return NULL;

	struct kn_list *tail = kn_list_alloc(list->length - 1);

	for (unsigned i = 0; i < list->length - 1; i++)
		tail->elements[i] = kn_value_clone(list->elements[i]);

	return tail;
}

struct kn_list *kn_list_concat(struct kn_list *lhs, struct kn_list *rhs) {
	if (!lhs->length) {
		assert(lhs == &kn_list_empty);
		return rhs;
	}

	if (KN_UNLIKELY(!rhs->length)) {
		assert(rhs == &kn_list_empty);
		return lhs;
	}

	struct kn_list *concat = kn_list_alloc(lhs->length + rhs->length);

	for (unsigned i = 0; i < lhs->length; ++i)
		concat->elements[i] = kn_value_clone(lhs->elements[i]);

	for (unsigned i = 0; i < rhs->length; ++i)
		concat->elements[i + lhs->length] = kn_value_clone(rhs->elements[i]);

	return concat;
}

struct kn_list *kn_list_repeat(struct kn_list *lhs, unsigned amount) {
	if (KN_UNLIKELY(amount == 0)) {
		kn_list_free(lhs);
		return &kn_list_empty;
	}

	if (KN_UNLIKELY(amount == 1))
		return lhs;

	struct kn_list *repeat = kn_list_alloc(lhs->length * amount);

	for (unsigned i = 0; i < amount; ++i)
		for (unsigned j = 0; j < lhs->length; ++j)
			repeat->elements[j + i*lhs->length] = kn_value_clone(lhs->elements[j]);

	kn_list_free(lhs);

	return repeat;
}

struct kn_string *kn_list_join(const struct kn_list *lhs, const struct kn_string *sep) {
	if (KN_UNLIKELY(!lhs->length)) {
		assert(lhs == &kn_list_empty);
		return &kn_string_empty;
	}

	if (KN_UNLIKELY(lhs->length == 1))
		return kn_value_to_string(lhs->elements[0]);

	unsigned len = 0, capacity = 64;
	char *joined = xmalloc(capacity);
	
	for (unsigned i = 0; i < lhs->length; ++i) {
		if (i) {
			if (capacity <= kn_string_length(sep) + len)
				joined = xrealloc(joined, capacity = capacity * 2 + kn_string_length(sep));

			memcpy(joined + len, kn_string_deref((struct kn_string *) sep), kn_string_length(sep));
			len += kn_string_length(sep);
		}

		struct kn_string *string = kn_value_to_string(lhs->elements[i]);
		if (capacity <= kn_string_length(string) + len)
			joined = xrealloc(joined, capacity = capacity * 2 + kn_string_length(string));
		memcpy(joined + len, kn_string_deref(string), kn_string_length(string));
		len += kn_string_length(string);
		kn_string_free(string);
	}

#ifndef NDEBUG
	joined = xrealloc(joined, len + 1);
	joined[len] = '\0';
#endif

	return kn_string_new_owned(joined, len);
}
