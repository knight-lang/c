#ifndef KN_LIST_H
#define KN_LIST_H

#include "value.h"  /* kn_value, kn_number, kn_boolean, kn_list */
#include "string.h"
#include <stddef.h> /* size_t */
#include <stdalign.h> /* alignas */

/*
 * How many additional `kn_value`s can be stored in an embedded list
 */
#ifndef KN_LIST_EMBED_PADDING
# define KN_LIST_EMBED_PADDING 1
#endif /* !KN_LIST_EMBED_PADDING */

#define KN_LIST_EMBED_LENGTH \
	(KN_LIST_EMBED_PADDING + (sizeof(struct kn_list *) * 2 / sizeof(kn_value)))

struct kn_list {
	alignas(8) size_t refcount;
	size_t length;

	// only `static` is are composable.
	enum kn_list_flags {
		KN_LIST_FL_EMBED  = 1,
		KN_LIST_FL_ALLOC  = 2,
		KN_LIST_FL_CONS   = 4,
		KN_LIST_FL_REPEAT = 8,
		KN_LIST_FL_STATIC = 16,
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

/*
 * Deallocates the memory associated with `string`; should only be called with
 * a string with a zero refcount.
 */
void kn_list_deallocate(struct kn_list *list);

static inline struct kn_list *kn_list_clone(struct kn_list *list) {
	assert(list->refcount != 0);

	++list->refcount;

	return list;
}

static inline void kn_list_free(struct kn_list *list) {
	assert(list->refcount != 0);

	if (--list->refcount == 0)
		kn_list_deallocate(list);
}

static inline kn_value kn_list_get(const struct kn_list *list, size_t index) {
	assert(index < list->length);

	if (KN_LIKELY(list->flags & KN_LIST_FL_EMBED))
		return list->embed[index];

	if (list->flags & KN_LIST_FL_ALLOC)
		return list->alloc[index];

	if (list->flags & KN_LIST_FL_CONS) {
		if (index < list->cons.lhs->length)
			return kn_list_get(list->cons.lhs, index);

		return kn_list_get(list->cons.rhs, index - list->cons.lhs->length);
	}

	if (list->flags & KN_LIST_FL_REPEAT)
		return kn_list_get(list->repeat.list, index % list->repeat.amount);

	KN_UNREACHABLE();
}

static inline void kn_list_set(
	struct kn_list *list,
	size_t index,
	kn_value value
) {
	assert(list->flags & (KN_LIST_FL_EMBED | KN_LIST_FL_ALLOC));
	assert(index < list->length);

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
	return list->length != 0;
}

static inline kn_number kn_list_to_number(const struct kn_list *list) {
	return (kn_number) list->length;
}

static inline struct kn_string *kn_list_to_string(const struct kn_list *list) {
	static struct kn_string newline = KN_STRING_NEW_EMBED("\n");
	return kn_list_join(list, &newline);
}

kn_number kn_list_compare(const struct kn_list *lhs, const struct kn_list *rhs);


#endif /* !KN_LIST_H */
