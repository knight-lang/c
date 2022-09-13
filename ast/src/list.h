#ifndef KN_LIST_H
#define KN_LIST_H

#include "value.h"  /* kn_value, kn_number, kn_boolean, kn_list */
#include "string.h"
#include <stddef.h> /* size_t */
#include <stdalign.h> /* alignas */

struct kn_list {
	alignas(8) unsigned refcount;
	unsigned length;
	kn_value *elements;
};

extern struct kn_list kn_list_empty;


struct kn_list *kn_list_alloc(size_t capacity);

/*
 * Deallocates the memory associated with `string`; should only be called with
 * a string with a zero refcount.
 */
void kn_list_deallocate(struct kn_list *list);

static inline struct kn_list *kn_list_clone(struct kn_list *list) {
	assert(list->refcount);
	++list->refcount;
	return list;
}

static inline void kn_list_free(struct kn_list *list) {
	assert(list->refcount);
	if (--list->refcount == 0)
		kn_list_deallocate(list);
}

void kn_list_dump(const struct kn_list *list, FILE *out);

struct kn_string *kn_list_join(const struct kn_list *list, const struct kn_string *sep);
kn_value kn_list_head(const struct kn_list *list);
struct kn_list *kn_list_tail(const struct kn_list *list);
struct kn_list *kn_list_concat(struct kn_list *lhs, struct kn_list *rhs);
struct kn_list *kn_list_repeat(struct kn_list *list, unsigned amount);
bool kn_list_equal(const struct kn_list *lhs, const struct kn_list *rhs);

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

#endif /* !KN_LIST_H */