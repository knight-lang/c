#ifndef KN_LIST_H
#define KN_LIST_H

#include "container.h"
#include "value.h"  /* kn_value, kn_integer, kn_boolean, kn_list */
#include "string.h"
#include <string.h>
#include <stddef.h> /* size_t */

/**
 * How many additional `kn_value`s can be stored in an embedded list.
 **/
#ifndef KN_LIST_EMBED_PADDING
# define KN_LIST_EMBED_PADDING 1
#endif /* !KN_LIST_EMBED_PADDING */

#define KN_LIST_EMBED_LENGTH \
	(KN_LIST_EMBED_PADDING + (sizeof(struct kn_list *) * 2 / sizeof(kn_value)))

struct kn_list {
	struct kn_container container;

	// only `static` is are composable.
	enum kn_list_flags {
		KN_LIST_FL_EMBED  = 1,
		KN_LIST_FL_ALLOC  = 2,
		KN_LIST_FL_CONS   = 4,
		KN_LIST_FL_REPEAT = 8,
		KN_LIST_FL_STATIC = 16,
		KN_LIST_FL_INTEGER = 32,
	} flags;

	union {
		kn_value embed[KN_LIST_EMBED_LENGTH];

		struct {
			struct kn_list *lhs, *rhs;
		} cons;

		struct {
			struct kn_list *list;
			size_t amount;
		} repeat;

		kn_value *alloc;
	};
};

extern struct kn_list kn_list_empty;

struct kn_list *kn_list_alloc(size_t length);

/**
 * Deallocates the memory associated with `string`; should only be called with
 * a string with a zero refcount.
 **/
void kn_list_dealloc(struct kn_list *list);

static inline struct kn_list *kn_list_clone(struct kn_list *list) {
	assert(kn_refcount(list) != 0);

	++kn_refcount(list);

	return list;
}

static inline struct kn_list *kn_list_clone_integer(struct kn_list *list) {
	if (!(list->flags & KN_LIST_FL_INTEGER))
		return list;

	struct kn_list *cloned = kn_list_alloc(kn_length(list));
	memcpy(
		(cloned->flags & KN_LIST_FL_EMBED)
			? cloned->embed
			: cloned->alloc,
		list->alloc,
		sizeof(kn_value) * kn_length(list)
	);
	return cloned;
}

static inline void kn_list_free(struct kn_list *list) {
	assert(kn_refcount(list) != 0);

	if (--kn_refcount(list) == 0)
		kn_list_dealloc(list);
}

static inline kn_value kn_list_get(const struct kn_list *list, size_t index) {
	assert(index < kn_length(list));

	if (KN_LIKELY(list->flags & KN_LIST_FL_EMBED))
		return list->embed[index];

	if (list->flags & KN_LIST_FL_ALLOC)
		return list->alloc[index];

	if (list->flags & KN_LIST_FL_CONS) {
		if (index < kn_length(list->cons.lhs))
			return kn_list_get(list->cons.lhs, index);

		return kn_list_get(list->cons.rhs, index - kn_length(list->cons.lhs));
	}

	if (list->flags & KN_LIST_FL_REPEAT)
		return kn_list_get(list->repeat.list, index % kn_length(list->repeat.list));

	KN_UNREACHABLE();
}

static inline void kn_list_set(
	struct kn_list *list,
	size_t index,
	kn_value value
) {
	assert(list->flags & (KN_LIST_FL_EMBED | KN_LIST_FL_ALLOC));
	assert(index < kn_length(list));

	(list->flags & KN_LIST_FL_EMBED ? list->embed : list->alloc)[index] = value;
}

void kn_list_dump(const struct kn_list *list, FILE *out);

struct kn_string *kn_list_join(
	const struct kn_list *list,
	const struct kn_string *sep
);

struct kn_list *kn_list_concat(struct kn_list *lhs, struct kn_list *rhs);
struct kn_list *kn_list_repeat(struct kn_list *list, size_t amount);
bool kn_list_equal(const struct kn_list *lhs, const struct kn_list *rhs);
struct kn_list *kn_list_get_sublist(
	struct kn_list *list, 
	size_t start,
	size_t length
);
struct kn_list *kn_list_set_sublist(
	struct kn_list *list,
	size_t start,
	size_t length,
	struct kn_list *replacement
);

static inline kn_boolean kn_list_to_boolean(const struct kn_list *list) {
	return kn_length(list) != 0;
}

static inline kn_integer kn_list_to_integer(const struct kn_list *list) {
	return (kn_integer) kn_length(list);
}

static inline struct kn_string *kn_list_to_string(const struct kn_list *list) {
	static struct kn_string newline = KN_STRING_NEW_EMBED("\n");
	return kn_list_join(list, &newline);
}

kn_integer kn_list_compare(const struct kn_list *lhs, const struct kn_list *rhs);


#endif /* !KN_LIST_H */
