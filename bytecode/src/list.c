extern void *memcpy(void *, const void *, unsigned long);
#include "list.h"
#include "alloc.h"
#define kn_length(x) (x)->length
#include "string.h"
#include <string.h>
#include <assert.h>

struct kn_list kn_list_empty = {
#ifdef KN_USE_REFCOUNT
	.refcount = 1,
#endif /* KN_USE_REFCOUNT */
	.flags = KN_LIST_FL_STATIC,
	.length = 0
};

static struct kn_list *alloc_list(size_t length, unsigned char flags) {
	struct kn_list *list = kn_heap_malloc(sizeof(struct kn_list));

#ifdef KN_USE_REFCOUNT
	list->refcount = 1;
#endif /* KN_USE_REFCOUNT */

	kn_length(list) = length;
	kn_flags(list) = flags;

	return list;
}

struct kn_list *kn_list_alloc(size_t length) {
	if (KN_UNLIKELY(length == 0))
		return &kn_list_empty;

	bool is_embed = KN_LIST_EMBED_LENGTH < length;
	struct kn_list *list = alloc_list(length, is_embed ? KN_LIST_FL_ALLOC : KN_LIST_FL_EMBED);

	if (is_embed)
		list->alloc = kn_heap_malloc(sizeof(kn_value) * length);

	return list;
}

void kn_list_dealloc(struct kn_list *list) {
	if (kn_flags(list) & KN_LIST_FL_STATIC)
		return;

#ifdef KN_USE_REFCOUNT
	assert(list->refcount == 0);
#endif /* KN_USE_REFCOUNT */

	assert(!(kn_flags(list) & KN_LIST_FL_INTEGER)); // `FL_STATIC` covers it.

	// since we're not `KN_LIST_FL_STATIC`, we can switch on them
	switch (kn_flags(list)) {
	case KN_LIST_FL_CONS:
		kn_list_dealloc(list->cons.lhs);
		kn_list_dealloc(list->cons.rhs);
		break;

	case KN_LIST_FL_REPEAT:
		kn_list_dealloc(list->repeat.list);
		break;

	case KN_LIST_FL_EMBED:
		for (size_t i = 0; i < kn_length(list); ++i)
			kn_value_dealloc(list->embed[i]); // TODO: is this valid?
		break;

	case KN_LIST_FL_ALLOC:
		for (size_t i = 0; i < kn_length(list); ++i)
			kn_value_dealloc(list->alloc[i]); // TODO: is this valid?

		kn_heap_free(list->alloc);
		break;

	default:
		KN_UNREACHABLE
	}

	kn_heap_free(list);
}

struct kn_list *kn_list_clone_integer(struct kn_list *list) {
	if (!(kn_flags(list) & KN_LIST_FL_INTEGER))
		return list;

	struct kn_list *cloned = kn_list_alloc(kn_length(list));
	kn_value *ptr = (kn_flags(cloned) & KN_LIST_FL_EMBED)
			? cloned->embed
			: cloned->alloc;

	memcpy(ptr, list->alloc, sizeof(kn_value) * kn_length(list));
	return cloned;
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
		kn_bug("todo");
		kn_integer cmp = 1;
		// kn_integer cmp = kn_value_compare(kn_list_get(lhs, i), kn_list_get(rhs, i));

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
		assert(!(kn_flags(lhs) & KN_LIST_FL_INTEGER));
		return lhs;
	}

	struct kn_list *concat = alloc_list(kn_length(lhs) + kn_length(rhs), KN_LIST_FL_CONS);
	concat->cons.lhs = lhs;
	concat->cons.rhs = rhs;

	return concat;
}

struct kn_list *kn_list_repeat(struct kn_list *list, size_t amount) {
	if (kn_length(list) == 0) {
		assert(list == &kn_list_empty);
		return list;
	}

	if (KN_UNLIKELY(amount == 0))
		return &kn_list_empty;

	if (KN_UNLIKELY(amount == 1))
		return list;


	struct kn_list *repetition = alloc_list(kn_length(list) * amount, KN_LIST_FL_REPEAT);
	repetition->repeat.list = list;
	repetition->repeat.amount = amount;

	return repetition;
}

struct kn_string *kn_list_join(const struct kn_list *list, const struct kn_string *sep) {
	if (kn_length(list) == 0) {
		assert(list == &kn_list_empty);
		return &kn_string_empty;
	}

	if (kn_length(list) == 1)
		return kn_value_to_string(kn_list_get(list, 0));

	size_t len = 0, cap = 64;
	char *joined = kn_heap_malloc(cap);
	
	for (size_t i = 0; i < kn_length(list); ++i) {
		if (i != 0) {
			if (cap <= kn_length(sep) + len)
				joined = kn_heap_realloc(joined, cap = cap * 2 + kn_length(sep));

			memcpy(joined + len, kn_string_deref(sep), kn_length(sep));
			len += kn_length(sep);
		}

		struct kn_string *string = kn_value_to_string(kn_list_get(list, i));
		if (cap <= kn_length(string) + len)
			joined = kn_heap_realloc(joined, cap = cap * 2 + kn_length(string));

		memcpy(joined + len, kn_string_deref(string), kn_length(string));
		len += kn_length(string);
	}

	joined = kn_heap_realloc(joined, len + 1);
	joined[len] = '\0';

	return kn_string_new_owned(joined, len);
}

struct kn_list *kn_list_get_sublist(struct kn_list *list, size_t start, size_t length) {
	assert(start + length <= kn_length(list));

	if (length == 0)
		return &kn_list_empty;

	if (KN_UNLIKELY(start == 0 && length == kn_length(list)))
		return list;

	struct kn_list *sublist = kn_list_alloc(length);

	for (size_t i = 0; i < length; ++i)
		kn_list_set(sublist, i, kn_list_get(list, i + start));

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
		kn_list_set(replaced, i, kn_list_get(list, i));

	for (size_t j = 0; j < kn_length(replacement); ++j, ++i)
		kn_list_set(replaced, i, kn_list_get(replacement, j));

	for (size_t j = start + length; j < kn_length(list); ++j, ++i)
		kn_list_set(replaced, i, kn_list_get(list, j));

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
