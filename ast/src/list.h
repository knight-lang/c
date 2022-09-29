#ifndef KN_LIST_H
#define KN_LIST_H

#include "container.h"
#include "value.h"
#include "string.h"

/**
 * How many additional `kn_value`s can be stored in an embedded list.
 **/
#ifndef KN_LIST_EMBED_PADDING
# define KN_LIST_EMBED_PADDING 1
#endif /* !KN_LIST_EMBED_PADDING */

/**
 * The amount of elements an embedded list can contain.
 **/
#define KN_LIST_EMBED_LENGTH \
	(KN_LIST_EMBED_PADDING + (sizeof(struct kn_list *) * 2 / sizeof(kn_value)))


/**
 * The list type in Knight.
 *
 * Generally, lists are allocated via `kn_list_alloc`. However, some of the builtin list functions
 * (such as `kn_list_concat`) will allocate specialized lists. Regardless of the list type, you can
 * query the length via `kn_length` and get elements via `kn_list_get`. 
 * 
 * Lists in Knight are entirely immutable---once created, their contents shouldn't change.
 *
 * Regardless of the type of list, it should be passed to `kn_list_free` to properly dispose of its
 * resources when you're done with it.
 * 
 * Note that the only list with length zero is `kn_list_empty`.
 **/
struct kn_list {
	/**
	 * The refcount and length of the list. 
	 **/
	struct kn_container container;

	/**
	 * Flags denoting how the list works.
	 * 
	 * Other than `KN_LIST_FL_STATIC` (and `KN_LIST_FL_INTEGER`), flags aren't composable.
	 */
	enum kn_list_flags {
		/**
		 * The list's elements are embedded directly within the list itself.
		 * 
		 * This corresponds to the `embed` variant.
		 **/
		KN_LIST_FL_EMBED = (1 << 0),

		/**
		 * The list's elements are located behind a pointer.
		 * 
		 * This corresponds to the `alloc` variant.
		 **/
		KN_LIST_FL_ALLOC = (1 << 1),

		/**
		 * The list's the concatenation of two other lists (via the `kn_list_concat` function).
		 * 
		 * This corresponds to the `cons` variant. (`cons` is a function that originally came from
		 * lisp and means concatenating two lists together)
		 **/
		KN_LIST_FL_CONS = (1 << 2),

		/**
		 * The list's the repetition of another list by a value greater than one.
		 * 
		 * This corresponds to the `repeat` variant.
		 **/
		KN_LIST_FL_REPEAT = (1 << 3),

		/**
		 * A mask to get all the "data representation" flags.
		 **/
		KN_LIST_FL_TYPE_MASK = (1 << 4) - 1,

		/**
		 * Indicates that a list is statically allocated, and should not be freed when
		 * `kn_list_dealloc` is called.
		 **/
		KN_LIST_FL_STATIC = (1 << 4),

		/**
		 * A special flag that's only ever used by the `kn_integer_to_list`. 
		 * 
		 * This is used to avoid allocating new lists each time an integer is converted to a list. To
		 * get an allocated version of this, use `kn_list_clone_integer`.
		 * 
		 * Note this flag is only ever used in conjunction with `KN_LIST_FL_STATIC`.
		 **/
		KN_LIST_FL_INTEGER = (1 << 5),
	} flags;

	union {
		/**
		 * Elements are embedded directly within a list's body. Corresponds to `KN_LIST_FL_EMBED`.
		 **/
		kn_value embed[KN_LIST_EMBED_LENGTH];

		/**
		 * Elements are referenced to a pointer. Corresponds to `KN_LIST_FL_ALLOC`.
		 **/
		kn_value *alloc;


		/**
		 * Elements are first from `lhs` then to `rhs`. Corresponds to `KN_LIST_FL_CONS`.
		 **/
		struct {
			struct kn_list *lhs, *rhs;
		} cons;

		/**
		 * Elements are the repetition of `list` `amount` times. Corresponds to `KN_LIST_FL_REPEAT`.
		 * 
		 * Note that `amount` is always greater than 1.
		 **/
		struct {
			struct kn_list *list;
			size_t amount;
		} repeat;
	};
};

/**
 * The empty list.
 * 
 * This is the only list with the length of zero. (All functions that would return an empty list
 * return this instead.)
 **/
extern struct kn_list kn_list_empty;

/**
 * Allocates a list that can hold `length` elements.
 * 
 * When you're done using it, call `kn_list_free`.
 **/
struct kn_list *kn_list_alloc(size_t length);

/**
 * Deallocates the memory associated with `string`; should only be called with
 * a string with a zero refcount.
 **/
void kn_list_dealloc(struct kn_list *list);

/**
 * Duplicates this list, returning another copy of it.
 *
 * Each copy must be `kn_list_free`d separately after use to ensure that no memory leaks occur.
 **/
static inline struct kn_list *kn_list_clone(struct kn_list *list) {
	assert(kn_refcount(list) != 0);

	++kn_refcount(list);

	return list;
}

/**
 * A no-op for normal lists, but duplicates `list` if it's an integer list.
 * 
 * Since integer lists are globally modifiable, if there's any possibility that a list's contents
 * will be overwritten, call this function.
 **/
struct kn_list *kn_list_clone_integer(struct kn_list *list);

/**
 * Indicates that the caller is done using this list.
 *
 * If this is the last live reference to the list, then `kn_list_dealloc` will be called.
 **/
static inline void kn_list_free(struct kn_list *list) {
	assert(kn_refcount(list) != 0);

	if (--kn_refcount(list) == 0)
		kn_list_dealloc(list);
}

/**
 * Gets the element at `index` from within `list`.
 * 
 * Note that `index` must be less than `list`'s length
 * 
 * This method exists because there's many different internal representations of `list`s---for
 * efficiency reasons---and so a unified interface for getting elements is needed. It is a
 * `static inline` function because it's so commonly used that when compiling without LTO, the speed
 * benefit from it being inlined is quite nice.
 **/
static inline kn_value kn_list_get(const struct kn_list *list, size_t index) {
	assert(index < kn_length(list));

	switch (list->flags & KN_LIST_FL_TYPE_MASK) {
	case KN_LIST_FL_EMBED:
		return list->embed[index];

	case KN_LIST_FL_ALLOC:
		return list->alloc[index];

	case KN_LIST_FL_CONS:
		if (index < kn_length(list->cons.lhs))
			return kn_list_get(list->cons.lhs, index);
		return kn_list_get(list->cons.rhs, index - kn_length(list->cons.lhs));

	case KN_LIST_FL_REPEAT:
		return kn_list_get(list->repeat.list, index % kn_length(list->repeat.list));

	default:
		KN_UNREACHABLE();
	}
}

/**
 * Sets an element within a list.
 **/
static inline void kn_list_set(struct kn_list *list, size_t index, kn_value value) {
	assert(list->flags & (KN_LIST_FL_EMBED | KN_LIST_FL_ALLOC));
	assert(index < kn_length(list));

	(list->flags & KN_LIST_FL_EMBED ? list->embed : list->alloc)[index] = value;
}

void kn_list_dump(const struct kn_list *list, FILE *out);

struct kn_string *kn_list_join(const struct kn_list *list, const struct kn_string *sep);

struct kn_list *kn_list_concat(struct kn_list *lhs, struct kn_list *rhs);
struct kn_list *kn_list_repeat(struct kn_list *list, size_t amount);
bool kn_list_equal(const struct kn_list *lhs, const struct kn_list *rhs);

struct kn_list *kn_list_get_sublist(struct kn_list *list, size_t start, size_t length);
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
