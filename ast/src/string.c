#include "string.h" /* prototypes, kn_string, kn_string_flags variants, size_t,
                       KN_STRING_NEW_EMBED */
#include "shared.h" /* xmalloc, kn_hash, KN_LIKELY, KN_UNLIKELY */
#include <stdlib.h> /* free, NULL */
#include <string.h> /* strlen, strcmp, memcpy */
#include <assert.h> /* assert */

// The empty string.
// we need the alignment for embedding.
struct kn_string _Alignas(16) kn_string_empty = KN_STRING_NEW_EMBED("");

#ifndef KN_STRING_CACHE_MAXLEN
# define KN_STRING_CACHE_MAXLEN 32
#endif /* !KN_STRING_CACHE_MAXLEN */

#ifndef KN_STRING_CACHE_LINESIZE
# define KN_STRING_CACHE_LINESIZE (1<<14)
#endif /* !KN_STRING_CACHE_LINESIZE */

static struct kn_string *string_cache[
	KN_STRING_CACHE_MAXLEN][KN_STRING_CACHE_LINESIZE];

static struct kn_string **get_cache_slot(const char *str, size_t length) {
	assert(length != 0);
	assert(length <= KN_STRING_CACHE_MAXLEN);

	unsigned long hash = kn_hash(str, length);

	return &string_cache[length - 1][hash & (KN_STRING_CACHE_LINESIZE - 1)];
}

size_t kn_string_length(const struct kn_string *string) {
	assert(string != NULL);

	return KN_LIKELY(string->flags & KN_STRING_FL_EMBED)
		? (size_t) string->embed.length
		: string->alloc.length;
}

char *kn_string_deref(struct kn_string *string) {
	assert(string != NULL);

	return KN_LIKELY(string->flags & KN_STRING_FL_EMBED)
		? string->embed.data
		: string->alloc.str;
}

struct kn_string *kn_string_alloc(size_t length) {
	if (KN_UNLIKELY(length == 0))
		return &kn_string_empty; // dont allocate empty structs

	struct kn_string *string = xmalloc(sizeof(struct kn_string));
	string->flags = KN_STRING_FL_STRUCT_ALLOC;
	string->refcount = 1;

	if (KN_LIKELY(length < KN_STRING_EMBEDDED_LENGTH)) {
		string->flags |= KN_STRING_FL_EMBED;
		string->embed.length = length;
	} else {
		string->alloc.length = length;
		string->alloc.str = xmalloc(length + 1); // + 1 for `'\0'`
	}

	return string;
}

// Allocate a `kn_string` and populate it with the given `str`.
static struct kn_string *allocated_heap_string(char *str, size_t length) {
	assert(str != NULL);
	assert(strlen(str) == length);
	assert(length != 0); // should have already been checked before.

	struct kn_string *string = xmalloc(sizeof(struct kn_string));

	string->flags = KN_STRING_FL_STRUCT_ALLOC;
	string->refcount = 1;
	string->alloc.length = length;
	string->alloc.str = str;

	return string;
}

// Actually deallocates the data associated with `string`.
static void deallocate_string(struct kn_string *string) {
	assert(string != NULL);
	assert(string->refcount == 0);

	// if we didn't allocate the struct, simply return.
	if (!(string->flags & KN_STRING_FL_STRUCT_ALLOC)) {
		assert(string->flags & (KN_STRING_FL_EMBED | KN_STRING_FL_STATIC));
		return;
	}

	// it's unlikely we're not embedded.
	if (KN_UNLIKELY(!(string->flags & KN_STRING_FL_EMBED)))
		free(string->alloc.str);

	free(string);
}

struct kn_string *kn_string_new_owned(char *str, size_t length) {
	// sanity check for inputs.
	assert(0 <= (ssize_t) length);
	assert(str != NULL);
	assert(strlen(str) == length);

	if (KN_UNLIKELY(length == 0)) {
		free(str); // free the owned string.
		return &kn_string_empty;
	}

	// if it's too big just dont cache it
	// (as it's unlikely to be referenced again)
	if (KN_STRING_CACHE_MAXLEN < length)
		return allocated_heap_string(str, length);

	struct kn_string **cacheline = get_cache_slot(str, length);
	struct kn_string *string = *cacheline;

	if (KN_LIKELY(string != NULL)) {
		// if it's the same as `str`, use the cached version.
		if (KN_LIKELY(strcmp(string->alloc.str, str) == 0)) {
			free(str); // we don't need this string anymore, free it.
			return kn_string_clone(string);
		}

 		if (string->refcount == 0)// if the string has no refcount, free it.
			deallocate_string(string);
	}

	return *cacheline = allocated_heap_string(str, length);
}

struct kn_string *kn_string_new_unowned(const char *str, size_t length) {
	if (KN_UNLIKELY(length == 0))
		return &kn_string_empty;

	if (KN_STRING_CACHE_MAXLEN < length)
		return allocated_heap_string(strndup(str, length), length);

	struct kn_string **cache = get_cache_slot(str, length);
	struct kn_string *string = *cache;

	if (KN_LIKELY(string != NULL)) {
		// if the string is the same, then that means we want the cached one.
		if (KN_LIKELY(strncmp(kn_string_deref(string), str, length) == 0))
			return kn_string_clone(string);

 		if (KN_LIKELY(string->refcount == 0))
			deallocate_string(string);
	};

	// it may be embeddable, so don't just call `allocated_heap_string`.
	*cache = string = kn_string_alloc(length);

	memcpy(kn_string_deref(string), str, length);
	kn_string_deref(string)[length] = '\0';

	return string;
}

void kn_string_free(struct kn_string *string) {
	assert(string != NULL);
	assert(string->refcount != 0); // can't free an already freed string...

	--string->refcount; // this is irrelevant for non-allocated strings.
}

struct kn_string *kn_string_clone(struct kn_string *string) {
	assert(string != NULL);
	assert(!(string->flags & KN_STRING_FL_STATIC));

	++string->refcount; // this is irrelevant for non-allocated structs.

	return string;
}

struct kn_string *kn_string_clone_static(struct kn_string *string) {
	if (!(string->flags & KN_STRING_FL_STATIC))
		return string;

	return kn_string_new_unowned(
		kn_string_deref(string),
		kn_string_length(string)
	);
}
