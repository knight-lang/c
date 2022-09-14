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
	if (KN_UNLIKELY(!capacity))
		return &kn_list_empty;

	struct kn_list *list = xmalloc(sizeof(struct kn_list));

	list->refcount = 1;
	list->length = capacity;
	list->elements = xmalloc(sizeof(kn_value) * capacity);

	return list;
}

struct kn_list *kn_list_box(kn_value value) {
	struct kn_list *list = kn_list_alloc(1);
	*list->elements = value;
	return list;
}

void kn_list_deallocate(struct kn_list *list) {
	assert(!list->refcount);

	if (KN_UNLIKELY(list == &kn_list_empty))
		return;

	for (unsigned i = 0; i < list->length; ++i)
		kn_value_free(list->elements[i]);

	free(list->elements);
	free(list);
}

bool kn_list_equal(const struct kn_list *lhs, const struct kn_list *rhs) {
	if (lhs->length != rhs->length)
		return false;

	for (unsigned i = 0; i < lhs->length; ++i)
		if (!kn_value_equal(lhs->elements[i], rhs->elements[i]))
			return false;

	return true;
}

kn_number kn_list_compare(const struct kn_list *lhs, const struct kn_list *rhs) {
	unsigned minlen = lhs->length < rhs->length ? lhs->length : rhs->length;

	kn_number cmp;
	for (unsigned i = 0; i < minlen; ++i)
		if ((cmp = kn_value_compare(lhs->elements[i], rhs->elements[i])))
			return cmp;

	return lhs->length - rhs->length;
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

	kn_list_free(lhs);
	kn_list_free(rhs);

	return concat;
}

struct kn_list *kn_list_repeat(struct kn_list *list, unsigned amount) {
	if (KN_UNLIKELY(amount == 0)) {
		kn_list_free(list);
		return &kn_list_empty;
	}

	if (KN_UNLIKELY(amount == 1))
		return list;

	struct kn_list *repeat = kn_list_alloc(list->length * amount);

	for (unsigned i = 0; i < amount; ++i)
		for (unsigned j = 0; j < list->length; ++j)
			repeat->elements[j + i*list->length] = kn_value_clone(list->elements[j]);

	kn_list_free(list);

	return repeat;
}

struct kn_string *kn_list_join(const struct kn_list *list, const struct kn_string *sep) {
	if (KN_UNLIKELY(!list->length)) {
		assert(list == &kn_list_empty);
		return &kn_string_empty;
	}

	if (list->length == 1)
		return kn_value_to_string(list->elements[0]);

	unsigned len = 0, cap = 64;
	char *joined = xmalloc(cap);
	
	for (unsigned i = 0; i < list->length; ++i) {
		if (i) {
			if (cap <= kn_string_length(sep) + len)
				joined = xrealloc(joined, cap = cap * 2 + kn_string_length(sep));

			memcpy(joined + len, kn_string_deref(sep), kn_string_length(sep));
			len += kn_string_length(sep);
		}

		struct kn_string *string = kn_value_to_string(list->elements[i]);
		if (cap <= kn_string_length(string) + len)
			joined = xrealloc(joined, cap = cap * 2 + kn_string_length(string));

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

struct kn_list *kn_list_get(struct kn_list *list, unsigned start, unsigned length) {
	assert(start + length <= list->length);

	if (!length) {
		kn_list_free(list);
		return &kn_list_empty;
	}

	if (KN_UNLIKELY(!start && length == list->length))
		return list;

	struct kn_list *sublist = kn_list_alloc(length);

	for (unsigned i = 0; i < length; ++i)
		sublist->elements[i] = kn_value_clone(list->elements[i + start]);

	kn_list_free(list);
	return sublist;
}

struct kn_list *kn_list_set(
	struct kn_list *list,
	unsigned start,
	unsigned length,
	struct kn_list *replacement
) {
	assert(start + length <= list->length);

	if (!length) {
		kn_list_free(list);
		return replacement;
	}

	struct kn_list *replaced = kn_list_alloc(list->length - length + replacement->length);

	unsigned i = 0;
	for (; i < start; ++i)
		replaced->elements[i] = kn_value_clone(list->elements[i]);
	for (unsigned j = 0; j < replacement->length; ++j, ++i)
		replaced->elements[i] = kn_value_clone(replacement->elements[j]);
	for (unsigned j = start + length; j < list->length; ++j, ++i)
		replaced->elements[i] = kn_value_clone(list->elements[i]);

	return replaced;
}

void kn_list_dump(const struct kn_list *list, FILE *out) {
	fputs("List(", out);

#ifndef KN_NPRETTY_PRINT_LISTS
	++kn_indentation;
#endif

	for (unsigned i = 0; i < list->length; ++i) {
		if (i)
			fputs(", ", out);

#ifndef KN_NPRETTY_PRINT_LISTS
		kn_indent(out);
#endif

		kn_value_dump(list->elements[i], out);
	}

#ifndef KN_NPRETTY_PRINT_LISTS
	--kn_indentation;
	kn_indent(out);
#endif

	fputc(')', out);
}
