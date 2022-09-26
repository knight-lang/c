#include "list.h"
#include "shared.h"
#include "string.h"
#include <string.h>
#include <assert.h>

struct kn_list kn_list_empty = {
	.container = {
		.refcount = { 1 },
		.length = 0
	},
	.flags = KN_LIST_FL_STATIC,
};

struct kn_list *kn_list_alloc(size_t length) {
	if (KN_UNLIKELY(length == 0))
		return &kn_list_empty;

	struct kn_list *list = xmalloc(sizeof(struct kn_list));

	*kn_refcount(list) = 1;
	kn_length(list) = length;

	if (KN_LIST_EMBED_LENGTH < length) {
		list->alloc = xmalloc(sizeof(kn_value) * length);
		list->flags = KN_LIST_FL_ALLOC;
	} else {
		list->flags = KN_LIST_FL_EMBED;
	}

	return list;
}

void kn_list_dealloc(struct kn_list *list) {
	if (list->flags & KN_LIST_FL_STATIC)
		return;

	assert(*kn_refcount(list) == 0);

	// since we're not `KN_LIST_FL_STATIC`, we can switch on them
	switch (list->flags) {
	case KN_LIST_FL_CONS:
		kn_list_free(list->cons.lhs);
		kn_list_free(list->cons.rhs);
		break;

	case KN_LIST_FL_REPEAT:
		kn_list_free(list->repeat.list);
		break;

	case KN_LIST_FL_EMBED:
		for (size_t i = 0; i < kn_length(list); ++i)
			kn_value_free(list->embed[i]);
		break;

	case KN_LIST_FL_ALLOC:
		for (size_t i = 0; i < kn_length(list); ++i)
			kn_value_free(list->alloc[i]);

		free(list->alloc);
		break;

	default:
		KN_UNREACHABLE();
	}

	free(list);
}

bool kn_list_equal(const struct kn_list *lhs, const struct kn_list *rhs) {
	if (lhs == rhs)
		return true;

	if (kn_length(lhs) != kn_length(rhs))
		return false;

	for (size_t i = 0; i < kn_length(lhs); ++i)
		if (!kn_value_equal(kn_list_get(lhs, i), kn_list_get(rhs, i)))
			return false;

	return true;
}

kn_integer kn_list_compare(const struct kn_list *lhs, const struct kn_list *rhs) {
	if (lhs == rhs)
		return 0;

	size_t minlen = kn_length(lhs) < kn_length(rhs) ? kn_length(lhs) : kn_length(rhs);

	for (size_t i = 0; i < minlen; ++i) {
		kn_integer cmp = kn_value_compare(
			kn_list_get(lhs, i),
			kn_list_get(rhs, i)
		);

		if (cmp != 0)
			return cmp;
	}

	return kn_length(lhs) - kn_length(rhs);
}

struct kn_list *kn_list_concat(struct kn_list *lhs, struct kn_list *rhs) {
	if (kn_length(lhs) == 0) {
		assert(lhs == &kn_list_empty);
		return kn_list_clone_integer(rhs);
	}

	if (KN_UNLIKELY(kn_length(rhs) == 0)) {
		assert(rhs == &kn_list_empty);
		assert(!(lhs->flags & KN_LIST_FL_INTEGER));
		return lhs;
	}

	struct kn_list *concat = xmalloc(sizeof(struct kn_list));

	*kn_refcount(concat) = 1;
	kn_length_set(concat, kn_length(lhs) + kn_length(rhs));
	concat->flags = KN_LIST_FL_CONS;
	concat->cons.lhs = lhs;
	concat->cons.rhs = rhs;

	return concat;
}

struct kn_list *kn_list_repeat(struct kn_list *list, size_t amount) {
	if (kn_length(list) == 0) {
		assert(list == &kn_list_empty);
		return list;
	}

	if (KN_UNLIKELY(amount == 0)) {
		kn_list_free(list);
		return &kn_list_empty;
	}

	if (KN_UNLIKELY(amount == 1))
		return list;


	struct kn_list *repetition = xmalloc(sizeof(struct kn_list));

	*kn_refcount(repetition) = 1;
	kn_length_set(repetition, kn_length(list) * amount);
	repetition->flags = KN_LIST_FL_REPEAT;
	repetition->repeat.list = list;
	repetition->repeat.amount = amount;

	return repetition;
}

struct kn_string *kn_list_join(
	const struct kn_list *list,
	const struct kn_string *sep
) {
	if (kn_length(list) == 0) {
		assert(list == &kn_list_empty);
		return &kn_string_empty;
	}

	if (kn_length(list) == 1)
		return kn_value_to_string(kn_list_get(list, 0));

	size_t len = 0, cap = 64;
	char *joined = xmalloc(cap);
	
	for (size_t i = 0; i < kn_length(list); ++i) {
		if (i != 0) {
			if (cap <= kn_length(sep) + len)
				joined = xrealloc(joined, cap = cap * 2 + kn_length(sep));

			memcpy(joined + len, kn_string_deref(sep), kn_length(sep));
			len += kn_length(sep);
		}

		struct kn_string *string = kn_value_to_string(kn_list_get(list, i));
		if (cap <= kn_length(string) + len)
			joined = xrealloc(joined, cap = cap * 2 + kn_length(string));

		memcpy(joined + len, kn_string_deref(string), kn_length(string));
		len += kn_length(string);

		kn_string_free(string);
	}

	joined = xrealloc(joined, len + 1);
	joined[len] = '\0';

	return kn_string_new_owned(joined, len);
}

struct kn_list *kn_list_get_sublist(
	struct kn_list *list,
	size_t start,
	size_t length
) {
	assert(start + length <= kn_length(list));

	if (length == 0) {
		kn_list_free(list);
		return &kn_list_empty;
	}

	if (KN_UNLIKELY(start == 0 && length == kn_length(list)))
		return list;

	struct kn_list *sublist = kn_list_alloc(length);

	for (size_t i = 0; i < length; ++i)
		kn_list_set(sublist, i, kn_value_clone(kn_list_get(list, i + start)));

	kn_list_free(list);
	return sublist;
}

struct kn_list *kn_list_set_sublist(
	struct kn_list *list,
	size_t start,
	size_t length,
	struct kn_list *replacement
) {
	assert(start + length <= kn_length(list));

	if (kn_length(list) == 0) {
		assert(list == &kn_list_empty);
		return replacement;
	}

	struct kn_list *replaced = kn_list_alloc(kn_length(list) - length + kn_length(replacement));

	size_t i = 0;
	for (; i < start; ++i)
		kn_list_set(replaced, i, kn_value_clone(kn_list_get(list, i)));

	for (size_t j = 0; j < kn_length(replacement); ++j, ++i)
		kn_list_set(replaced, i, kn_value_clone(kn_list_get(replacement, j)));

	for (size_t j = start + length; j < kn_length(list); ++j, ++i)
		kn_list_set(replaced, i, kn_value_clone(kn_list_get(list, j)));

	return replaced;
}

void kn_list_dump(const struct kn_list *list, FILE *out) {
	fputc('[', out);

	for (size_t i = 0; i < kn_length(list); ++i) {
		if (i != 0)
			fputs(", ", out);

		kn_value_dump(kn_list_get(list, i), out);
	}

	fputc(']', out);
}
