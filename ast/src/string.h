#ifndef KN_STRING_H
#define KN_STRING_H

#include <stddef.h> /* size_t */

/*
 * These flags are used to record information about how the memory of a
 * `kn_string` should be managed.
 */
enum kn_string_flags {
	/*
	 * Indicates that the struct itself was allocated.
	 *
	 * If this is set, when a string is `kn_string_free`d, the struct pointer
	 * itself will also be freed.
	 */
	KN_STRING_FL_STRUCT_ALLOC = 1,

	/*
	 * Indicates that a string's data is stored in the `embed`ded field of
	 * the string, rather than the `alloc`ated  part.
	 */
	KN_STRING_FL_EMBED = 2,

	/*
	 * Indicates that the string is a `static` string---that is, it's not
	 * allocated, but should it should be fully duplicated when the function
	 * `kn_string_clone_static` is called.
	 */
	KN_STRING_FL_STATIC = 4,

	/*
	 * Indicates that the string is cached, and thus should not delete itself
	 * when its `refcount` goes to zero.
	 */
	KN_STRING_FL_CACHED = 8
};

/*
 * How many bytes of padding to use; the larger the number, the more strings are
 * embedded, but the more memory used.
 */
#ifndef KN_STRING_PADDING_LENGTH
# define KN_STRING_PADDING_LENGTH 16
#endif /* !KN_STRING_PADDING_LENGTH */

/*
 * The length of the embedded segment of the string.
 */
#define KN_STRING_EMBEDDED_LENGTH \
	(sizeof(size_t) \
		+ sizeof(char *) \
		+ sizeof(char [KN_STRING_PADDING_LENGTH]) \
		- 1) // for null ptr

/*
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
 */
struct kn_string {
	/*
	 * The flags that dictate how to manage this struct's memory.
	 *
	 * Note that the struct _must_ have an 8-bit alignment, so as to work with
	 * `kn_value`'s layout.
	 */
	_Alignas(8) enum kn_string_flags flags;

	/*
	 * The amount of references to this string.
	 *
	 * This is increased when `kn_string_clone`d and decreased when
	 * `kn_string_free`d, and when it reaches zero, the struct will be freed.
	 */
	unsigned refcount;

	/* All strings are either embedded or allocated. */
	union {
		struct {
			/*
			 * The length of the embedded string.
			 */
			char length;

			/*
			 * The actual data for the embedded string.
			 */
			char data[KN_STRING_EMBEDDED_LENGTH];
		} embed;

		struct {
			/*
			 * The length of the allocated string.
			 *
			 * This should equal `strlen(str)`, and is just an optimization aid.
			 */
			size_t length;

			/*
			 * The data for an allocated string.
			 */
			char *str;
		} alloc;
	};

	/*
	 * Extra padding for the struct, to make embedded strings have more room.
	 *
	 * This is generally a number that makes this struct's size an even multiple
	 * of two (so as to fill the space an allocator gives us).
	 */
	char _padding[KN_STRING_PADDING_LENGTH];
};

/*
 * The empty string.
 */
extern struct kn_string kn_string_empty;

/*
 * A macro to create a new embedded struct.
 *
 * It's up to the caller to ensure that `data_` can fit within an embedded
 * string.
 */
#define KN_STRING_NEW_EMBED(data_) \
	{ \
		.flags = KN_STRING_FL_EMBED, \
		.embed = { .length = sizeof(data_) - 1, .data = data_ } \
	}

/*
 * Allocates a new `kn_string` that can hold at least the given length.
 *
 * Note that the return value of this is not yet cached, and after the struct is
 * done being populated, it should be passed to `kn_string_cache` to enable
 * future structs to use it.
 */
struct kn_string *kn_string_alloc(size_t length);

/*
 * Caches a string, such that `kn_string_new_xxx` can possibly use the string
 * in the future.
 *
 * This should only be used in conjunction with `kn_string_alloc`, as the other
 * string creation functions will automatically cache any strings they create.
 */
void kn_string_cache(struct kn_string *string);

/*
 * Creates a new `kn_string` of the given length, and then initializes it to
 * `str`; the `str`'s ownership should be given given to this function.
 *
 * Note that `length` should equal `strlen(str)`.
 */
struct kn_string *kn_string_new_owned(char *str, size_t length);

/*
 * Creates a new `kn_string` of the given unowned `str` (of length `length`).
 * The given string's ownership is not passed to the function.
 *
 * Note that, unlike `kn_string_new_owned`, `length` does _not_ have to be the
 * length of `str`--it can be smaller.
 */
struct kn_string *kn_string_new_borrowed(const char *str, size_t length);

/*
 * Looks up the string corresponding to the given `hash`.
 *
 * This function's a bit hacky, and probably could be redesigned a bit better...
 */
struct kn_string *kn_string_cache_lookup(unsigned long hash, size_t length);

/*
 * Returns the length of the string, in bytes.
 */
size_t kn_string_length(const struct kn_string *string);

/*
 * Dereferences the string, returning a mutable pointer to its data.
 */
char *kn_string_deref(struct kn_string *string);

/*
 * Duplicates this string, returning another copy of it.
 *
 * Each copy must be `kn_string_free`d separately after use to ensure that no
 * memory errors occur.
 */
struct kn_string *kn_string_clone(struct kn_string *string);

/*
 * Duplicates `KN_STRING_FL_STATIC` strings, simply returns all others.
 *
 * This is intended to be used for strings that are `static`---they normally
 * don't need to be heap-allocated (eg if they're simply being printed out), but
 * if an entire new string is needed, this is used to ensure that the returned
 * string won't change if the original static one does.
 */
struct kn_string *kn_string_clone_static(struct kn_string *string);

/*
 * Indicates that the caller is done using this string.
 *
 * If this is the last live reference to the string, and the string is not
 * cached (generally due to it being evicted whilst a live reference still
 * existed), then the struct itself will be deallocated. However, if the string
 * is cached, it actually won't be deallocated until it's evicted---that way,
 * we don't end up allocating multiple times for the same string.
 */
void kn_string_free(struct kn_string *string);

#endif /* !KN_STRING_H */
