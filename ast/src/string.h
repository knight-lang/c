#ifndef KN_STRING_H
#define KN_STRING_H

#include <stddef.h>   /* size_t */
#include <stdbool.h>  /* bool */
#include "decls.h"
#include <stdio.h>
#include "shared.h"
#include "container.h"
#include <assert.h>

/**
 * These flags are used to record information about how the memory of a
 * `kn_string` should be managed.
 **/
/*
 * The flags that dictate how to manage this struct's memory.
 *
 * Note that the struct _must_ have an 8-bit alignment, so as to work with
 * `kn_value`'s layout.
 */
enum kn_string_flags {
	/*
	 * Indicates that the struct itself was allocated.
	 *
	 * If this is set, when a string is `kn_string_free`d, the struct pointer
	 * itself will also be freed.
	 */
	KN_STRING_FL_STRUCT_ALLOC = (1 << 0),

	/*
	 * Indicates that a string's data is stored in the `embed`ded field of
	 * the string, rather than the `alloc`ated  part.
	 */
	KN_STRING_FL_EMBED = (1 << 1),

	/*
	 * Indicates that the string is a `static` string---that is, it's not
	 * allocated, but should it should be fully duplicated when the function
	 * `kn_string_clone_static` is called.
	 */
	KN_STRING_FL_STATIC = (1 << 2)

#ifdef KN_STRING_CACHE
	/*
	 * Indicates that the string is cached, and thus should not delete itself
	 * when its `refcount` goes to zero.
	 */
	, KN_STRING_FL_CACHED = (1 << 3)
#endif /* KN_STRING_CACHE */

#ifdef KN_USE_GC
	, KN_LIST_FL_MARK = KN_GC_FL_MARKED
#endif /* KN_USE_GC */
};

/**
 * How many bytes of padding to use; the larger the number, the more strings are
 * embedded, but the more memory used.
 **/
#ifndef KN_STRING_PADDING_LENGTH
# define KN_STRING_PADDING_LENGTH 16
#endif /* !KN_STRING_PADDING_LENGTH */

/**
 * The length of the embedded segment of the string.
 **/
#define KN_STRING_EMBEDDED_LENGTH \
	(sizeof(size_t) \
		+ sizeof(char *) \
		+ sizeof(char [KN_STRING_PADDING_LENGTH]) \
		- 1) // for null ptr

/**
 * The string type in Knight.
 *
 * This struct is generally allocated by a `kn_string_alloc`, which can then be
 * populated via the `kn_string_deref` function.
 *
 * As an optimization, strings can either be `embed`ded (where the `chars` are
 * actually a part of the struct), or `alloc`ated (where the data is stored
 * elsewhere, and a pointer to it is used.)
 *
 * Regardless of the type of string, it should be passed to `kn_string_free` to
 * properly dispose of its resources when you're finished with it.
 **/
struct kn_string {
	struct kn_container container;

	/* All strings are either embedded or allocated. */
	union {
		/*
		 * The actual data for the embedded string.
		 */
		char embed[KN_STRING_EMBEDDED_LENGTH];

		/*
		 * The data for an allocated string.
		 */
		char *ptr;
	};
};

/**
 * The empty string.
 **/
extern struct kn_string kn_string_empty;

/**
 * A macro to create a new embedded struct.
 *
 * It's up to the caller to ensure that `data` can fit within an embedded
 * string.
 **/
#define KN_STRING_NEW_EMBED(data)                            \
	{                                                    \
		.container = {                               \
			.header = {                          \
				.flags = KN_STRING_FL_EMBED, \
			},                                   \
			.length = sizeof(data) - 1           \
		},                                           \
		.embed = data                                \
	}

/**
 * Frees cached strings with a zero refcount.
 *
 * To improve performance, string structs with a zero refcount aren't actually
 * deallocated until they are evicted by another string with the same hash.
 * However, this means that inaccessible strings are left in memory. This
 * function is used to clean up the memory associated with these zombie strings.
 *
 * Note that this will _not_ clean up live strings, but only ones with a
 * refcount of zero.
 **/
void KN_COLD kn_string_cleanup(void);

/**
 * Allocates a new `kn_string` that can hold at least the given length.
 *
 * Note that the return value of this is not yet cached, and after the struct is
 * done being populated, it should be passed to `kn_string_cache` to enable
 * future structs to use it.
 **/
struct kn_string *kn_string_alloc(size_t length);

/**
 * Creates a new `kn_string` of the given length, and then initializes it to
 * `str`; the `str`'s ownership should be given given to this function.
 *
 * Note that `length` should equal `strlen(str)`.
 **/
struct kn_string *kn_string_new_owned(char *str, size_t length);

/**
 * Creates a new `kn_string` of the given unowned `str` (of length `length`).
 * The given string's ownership is not passed to the function.
 *
 * Note that, unlike `kn_string_new_owned`, `length` does _not_ have to be the
 * length of `str`--it can be smaller.
 **/
struct kn_string *kn_string_new_borrowed(const char *str, size_t length);


#ifdef KN_STRING_CACHE
/**
 * Caches a string, such that `kn_string_new_xxx` can possibly use the string
 * in the future.
 *
 * This should only be used in conjunction with `kn_string_alloc`, as the other
 * string creation functions will automatically cache any strings they create.
 **/
void kn_string_cache(struct kn_string *string);

/**
 * Looks up the string corresponding to the given `hash`.
 *
 * This function's a bit hacky, and probably could be redesigned a bit better...
 **/
struct kn_string *kn_string_cache_lookup(kn_hash_t hash, size_t length);
#endif /* KN_STRING_CACHE */

/**
 * Dereferences the string, returning a mutable/immutable pointer to its data.
 **/
#define kn_string_deref(x) (_Generic(x,             \
	const struct kn_string *: kn_string_deref_const, \
	      struct kn_string *: kn_string_deref_mut    \
	)(x))

static inline char *kn_string_deref_mut(struct kn_string *string) {
	return KN_LIKELY(kn_flags(string) & KN_STRING_FL_EMBED)
		? string->embed
		: string->ptr;
}

static inline const char *kn_string_deref_const(
	const struct kn_string *string
) {
	return KN_LIKELY(kn_flags(string) & KN_STRING_FL_EMBED)
		? string->embed
		: string->ptr;
}

/**
 * Duplicates this string, returning another copy of it.
 *
 * Each copy must be `kn_string_free`d separately after use to ensure that no
 * memory errors occur.
 **/
static inline struct kn_string *kn_string_clone(struct kn_string *string) {
	assert(!(kn_flags(string) & KN_STRING_FL_STATIC));

#ifdef KN_USE_REFCOUNT
	++kn_refcount(string); // this is irrelevant for non-allocated structs.
#endif /* KN_USE_REFCOUNT */

	return string;
}


/**
 * Duplicates `KN_STRING_FL_STATIC` strings, simply returns all others.
 *
 * This is intended to be used for strings that are `static`---they normally
 * don't need to be heap-allocated (eg if they're simply being printed out), but
 * if an entire new string is needed, this is used to ensure that the returned
 * string won't change if the original static one does.
 **/
#ifndef KN_USE_REFCOUNT
static inline struct kn_string *kn_string_clone_static(struct kn_string *string) {
	return string;
}
#else
struct kn_string *kn_string_clone_static(struct kn_string *string);
#endif

/**
 * Deallocates the memory associated with `string`; should only be called with
 * a string with a zero refcount.
 **/
void kn_string_dealloc(struct kn_string *string);

/**
 * Indicates that the caller is done using this string.
 *
 * If this is the last live reference to the string, and the string is not
 * cached (generally due to it being evicted whilst a live reference still
 * existed), then the struct itself will be deallocated. However, if the string
 * is cached, it actually won't be deallocated until it's evicted---that way,
 * we don't end up allocating multiple times for the same string.
 **/
static inline void kn_string_free(struct kn_string *string) {
#ifndef kn_refcount
	(void) string;
#else
	if (--kn_refcount(string) == 0)
		kn_string_dealloc(string);
#endif /* !kn_refcount */
}

/**
 * Checks to see if two strings have the same contents.
 **/
bool kn_string_equal(const struct kn_string *lhs, const struct kn_string *rhs);

kn_integer kn_string_compare(const struct kn_string *lhs, const struct kn_string *rhs);

/**
 * Convert a string to an integer, as per the knight specs.
 *
 * This means we strip all leading whitespace, and then an optional `-` or `+`
 * may appear (`+` is ignored, `-` indicates a negative integer). Then as many
 * digits as possible are read.
 **/
static inline kn_integer kn_string_to_integer(const struct kn_string *string) {
	return strtoll(kn_string_deref(string), NULL, 10);
}

struct kn_list *kn_string_to_list(const struct kn_string *string);
struct kn_string *kn_string_concat(struct kn_string *lhs, struct kn_string *rhs);
struct kn_string *kn_string_repeat(struct kn_string *string, size_t amount);
struct kn_string *kn_string_get_substring(struct kn_string *string, size_t start, size_t length);
struct kn_string *kn_string_set_substring(
	struct kn_string *string,
	size_t start,
	size_t length,
	struct kn_string *replacement
);

void kn_string_dump(const struct kn_string *string, FILE *out);

#endif /* !KN_STRING_H */
