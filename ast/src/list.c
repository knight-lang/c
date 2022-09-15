#include "list.h"
#include "shared.h"
#include "string.h"
#include <string.h>
#include <assert.h>

struct kn_list kn_list_empty = {
	.refcount = 1,
	.length = 0,
	.flags = KN_LIST_FL_STATIC,
};

struct kn_list *kn_list_alloc(size_t length) {
	if (KN_UNLIKELY(length == 0))
		return &kn_list_empty;

	struct kn_list *list = xmalloc(sizeof(struct kn_list));

	list->refcount = 1;
	list->length = length;

	if (KN_LIST_EMBED_LENGTH < length) {
		list->alloc = xmalloc(sizeof(kn_value) * length);
		list->flags = KN_LIST_FL_ALLOC;
	} else {
		list->flags = KN_LIST_FL_EMBED;
	}

	return list;
}

void kn_list_deallocate(struct kn_list *list) {
	if (list->flags & KN_LIST_FL_STATIC)
		return;

	assert(list->refcount == 0);

	if (list->flags & KN_LIST_FL_CONS) {
		kn_list_free(list->cons.lhs);
		kn_list_free(list->cons.rhs);
	} else if (list->flags & KN_LIST_FL_REPEAT) {
		kn_list_free(list->repeat.list);
	} else if (list->flags & KN_LIST_FL_EMBED) {
		for (size_t i = 0; i < list->length; ++i)
			kn_value_free(list->embed[i]);
	} else {
		assert(list->flags & KN_LIST_FL_ALLOC);

		for (size_t i = 0; i < list->length; ++i)
			kn_value_free(list->alloc[i]);

		free(list->alloc);
	}

	free(list);
}

bool kn_list_equal(const struct kn_list *lhs, const struct kn_list *rhs) {
	if (lhs == rhs)
		return true;

	if (lhs->length != rhs->length)
		return false;

	for (size_t i = 0; i < lhs->length; ++i)
		if (!kn_value_equal(kn_list_get(lhs, i), kn_list_get(rhs, i)))
			return false;

	return true;
}

kn_number kn_list_compare(const struct kn_list *lhs, const struct kn_list *rhs) {
	if (lhs == rhs)
		return 0;

	size_t minlen = lhs->length < rhs->length ? lhs->length : rhs->length;

	kn_number cmp;
	for (size_t i = 0; i < minlen; ++i)
		if ((cmp = kn_value_compare(kn_list_get(lhs, i), kn_list_get(rhs, i))))
			return cmp;

	return lhs->length - rhs->length;
}

struct kn_list *kn_list_concat(struct kn_list *lhs, struct kn_list *rhs) {
	if (lhs->length == 0) {
		assert(lhs == &kn_list_empty);
		return rhs;
	}

	if (KN_UNLIKELY(rhs->length == 0)) {
		assert(rhs == &kn_list_empty);
		return lhs;
	}

	struct kn_list *concat = xmalloc(sizeof(struct kn_list));
	concat->refcount = 1;
	concat->length = lhs->length + rhs->length;
	concat->flags = KN_LIST_FL_CONS;
	concat->cons.lhs = lhs;
	concat->cons.rhs = rhs;
	return concat;
}

struct kn_list *kn_list_repeat(struct kn_list *list, size_t amount) {
	if (KN_UNLIKELY(amount == 0)) {
		kn_list_free(list);
		return &kn_list_empty;
	}

	if (KN_UNLIKELY(amount == 1))
		return list;


	struct kn_list *repetition = xmalloc(sizeof(struct kn_list));
	repetition->refcount = 1;
	repetition->length = list->length * amount;
	repetition->flags = KN_LIST_FL_REPEAT;
	repetition->repeat.list = list;
	repetition->repeat.amount = amount;
	return repetition;
}

struct kn_string *kn_list_join(const struct kn_list *list, const struct kn_string *sep) {
	if (KN_UNLIKELY(list->length == 0)) {
		assert(list == &kn_list_empty);
		return &kn_string_empty;
	}

	if (list->length == 1)
		return kn_value_to_string(kn_list_get(list, 0));

	size_t len = 0, cap = 64;
	char *joined = xmalloc(cap);
	
	for (size_t i = 0; i < list->length; ++i) {
		if (i) {
			if (cap <= sep->length + len)
				joined = xrealloc(joined, cap = cap * 2 + sep->length);

			memcpy(joined + len, kn_string_deref(sep), sep->length);
			len += sep->length;
		}

		struct kn_string *string = kn_value_to_string(kn_list_get(list, i));
		if (cap <= string->length + len)
			joined = xrealloc(joined, cap = cap * 2 + string->length);

		memcpy(joined + len, kn_string_deref(string), string->length);
		len += string->length;

		kn_string_free(string);
	}

#ifndef NDEBUG
	joined = xrealloc(joined, len + 1);
	joined[len] = '\0';
#endif

	return kn_string_new_owned(joined, len);
}

struct kn_list *kn_list_get_sublist(struct kn_list *list, size_t start, size_t length) {
	assert(start + length <= list->length);

	if (length == 0) {
		kn_list_free(list);
		return &kn_list_empty;
	}

	if (KN_UNLIKELY(start == 0 && length == list->length))
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
	assert(start + length <= list->length);

	if (list->length == 0) {
		assert(list == &kn_list_empty);
		return replacement;
	}

	struct kn_list *replaced = kn_list_alloc(list->length - length + replacement->length);

	size_t i = 0;
	for (; i < start; ++i)
		kn_list_set(replaced, i, kn_value_clone(kn_list_get(list, i)));

	for (size_t j = 0; j < replacement->length; ++j, ++i)
		kn_list_set(replaced, i, kn_value_clone(kn_list_get(replacement, j)));

	for (size_t j = start + length; j < list->length; ++j, ++i)
		kn_list_set(replaced, i, kn_value_clone(kn_list_get(list, j)));

	return replaced;
}

void kn_list_dump(const struct kn_list *list, FILE *out) {
	fputs("List(", out);

#ifndef KN_NPRETTY_PRINT_LISTS
	++kn_indentation;
#endif

	for (size_t i = 0; i < list->length; ++i) {
		if (i)
			fputs(", ", out);

#ifndef KN_NPRETTY_PRINT_LISTS
		kn_indent(out);
#endif

		kn_value_dump(kn_list_get(list, i), out);
	}

#ifndef KN_NPRETTY_PRINT_LISTS
	--kn_indentation;
	kn_indent(out);
#endif

	fputc(')', out);
}
