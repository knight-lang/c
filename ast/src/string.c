#include "string.h" /* prototypes, kn_string, kn_string_flags variants, size_t,
                       KN_STRING_NEW_EMBED, alignas */
#include "shared.h" /* xmalloc, kn_hash, KN_LIKELY, KN_UNLIKELY */
#include <stdlib.h> /* free, NULL */
#include <string.h> /* strlen, strcmp, memcpy, strndup, strncmp, memcmp */
#include <assert.h> /* assert */
#include <ctype.h>
#include "list.h"

// The empty string.
// we need the alignment for embedding.
struct kn_string alignas(8) kn_string_empty = KN_STRING_NEW_EMBED("");

#ifdef KN_STRING_CACHE
# ifndef KN_STRING_CACHE_MAXLEN
#  define KN_STRING_CACHE_MAXLEN 32
# endif /* !KN_STRING_CACHE_MAXLEN */
# ifndef KN_STRING_CACHE_LINELEN
#  define KN_STRING_CACHE_LINELEN (1<<14)
# endif /* !KN_STRING_CACHE_LINELEN */

static struct kn_string *cache[KN_STRING_CACHE_MAXLEN][KN_STRING_CACHE_LINELEN];

static struct kn_string **cache_lookup(unsigned long hash, size_t length) {
	assert(length != 0);
	assert(length <= KN_STRING_CACHE_MAXLEN);

	return &cache[length - 1][hash & (KN_STRING_CACHE_LINELEN - 1)];
}

struct kn_string *kn_string_cache_lookup(unsigned long hash, size_t length) {
	if (length == 0 || KN_STRING_CACHE_MAXLEN < length)
		return NULL;

	return *cache_lookup(hash, length);
}

static struct kn_string **get_cache_slot(const char *str, size_t length) {
	return cache_lookup(kn_hash(str, length), length);
}
#endif /* KN_STRING_CACHE */

size_t kn_string_length(const struct kn_string *string) {
	assert(string != NULL);

	return (size_t) string->length;
}

char *kn_string_deref_mut(struct kn_string *string) {
	assert(string != NULL);

	return KN_LIKELY(string->flags & KN_STRING_FL_EMBED)
		? string->embed
		: string->ptr;
}

const char *kn_string_deref_const(const struct kn_string *string) {
	assert(string != NULL);

	return KN_LIKELY(string->flags & KN_STRING_FL_EMBED)
		? string->embed
		: string->ptr;
}

bool kn_string_equal(const struct kn_string *lhs, const struct kn_string *rhs) {
	if (lhs == rhs) // shortcut if they have the same pointer.
		return true;

	if (kn_string_length(lhs) != kn_string_length(rhs))
		return false;

	return !memcmp(kn_string_deref(lhs), kn_string_deref(rhs), kn_string_length(lhs));
}

// Allocate a `kn_string` and populate it with the given `str`.
static struct kn_string *allocate_heap_string(char *str, size_t length) {
	assert(str != NULL);
	assert(length != 0); // zero length strings are `empty`.

	struct kn_string *string = xmalloc(sizeof(struct kn_string));

	string->flags = KN_STRING_FL_STRUCT_ALLOC;
	string->refcount = 1;
	string->length = length;
	string->ptr = str;

	return string;
}

static struct kn_string *allocate_embed_string(size_t length) {
	assert(length != 0);

	struct kn_string *string = xmalloc(sizeof(struct kn_string));

	string->flags = KN_STRING_FL_STRUCT_ALLOC | KN_STRING_FL_EMBED;
	string->refcount = 1;
	string->length = length;

	return string;
}

// Actually deallocates the data associated with `string`.
static void deallocate_string(struct kn_string *string) {
	assert(string != NULL);
	assert(string->refcount == 0); // don't dealloc live strings...

	// If the struct isn't actually allocated, then return.
	if (!(string->flags & KN_STRING_FL_STRUCT_ALLOC)) {
		// Sanity check, as these are the only two non-struct-ptr flags.
		assert(string->flags & (KN_STRING_FL_EMBED | KN_STRING_FL_STATIC));
		return;
	}

	// If we're not embedded, free the allocated string
	if (KN_UNLIKELY(!(string->flags & KN_STRING_FL_EMBED)))
		free(string->ptr);

	// Finally free the entire struct itself.
	free(string);
}

static void evict_string_active(struct kn_string *string) {
	assert(string->refcount != 0);
	assert(string->flags & KN_STRING_FL_CACHED);

	string->flags -= KN_STRING_FL_CACHED;
}

static void evict_string(struct kn_string *string) {
	// we only cache allocated strings.
	assert(string->flags & KN_STRING_FL_STRUCT_ALLOC);

	if (string->refcount == 0) {
		// If there are no more references to it, deallocate the string.
		deallocate_string(string);
	} else {
		// otherwise, just evict it from the cache slot.
		evict_string_active(string);
	}
}

struct kn_string *kn_string_alloc(size_t length) {
	// If it has no length, return the empty string
	if (KN_UNLIKELY(length == 0))
		return &kn_string_empty;

	// If it's embeddable, then allocate an embedded one.
	if (KN_LIKELY(length <= KN_STRING_EMBEDDED_LENGTH))
		return allocate_embed_string(length);

	// If it's too large to embed, heap allocate it with an uninit buffer.
	return allocate_heap_string(xmalloc(length + 1), length);
}

// Cache a string. note that it could have previously een cached.
void kn_string_cache(struct kn_string *string) {
	// empty strings should never be cached.
	assert(kn_string_length(string) != 0);

	// If it's too large for the cache, then just ignore it.
	if (KN_STRING_CACHE_MAXLEN < kn_string_length(string))
		return;

	struct kn_string **cacheline = get_cache_slot(
		kn_string_deref(string),
		kn_string_length(string)
	);

	// If there was something there and has no live references left, free it.
	if (KN_LIKELY(*cacheline != NULL))
		evict_string(*cacheline);

	// Indicate that the string is now cached, and replace the old cache line.
	string->flags |= KN_STRING_FL_CACHED;
	*cacheline = string;
}

struct kn_string *kn_string_new_owned(char *str, size_t length) {
	// sanity check for inputs.
	assert(str != NULL);
	assert(strlen(str) == length);

	// If the input is empty, then just return an owned string.
	if (KN_UNLIKELY(length == 0)) {
		free(str); // free the owned string.
		return &kn_string_empty;
	}

	// if it's too big just dont cache it
	if (KN_STRING_CACHE_MAXLEN < length)
		return allocate_heap_string(str, length);

	struct kn_string **cacheline = get_cache_slot(str, length);
	struct kn_string *string = *cacheline;

	if (KN_LIKELY(string != NULL)) {
		// if it's the same as `str`, use the cached version.
		if (KN_LIKELY(strcmp(kn_string_deref(string), str) == 0)) {
			free(str); // we don't need this string anymore, free it.
			return kn_string_clone(string);
		}

		evict_string(string);
	}

	*cacheline = string = allocate_heap_string(str, length);
	string->flags |= KN_STRING_FL_CACHED;

	return string;
}

struct kn_string *kn_string_new_borrowed(const char *str, size_t length) {
	if (KN_UNLIKELY(length == 0))
		return &kn_string_empty;

	if (KN_STRING_CACHE_MAXLEN < length)
		return allocate_heap_string(strndup(str, length), length);

	struct kn_string **cache = get_cache_slot(str, length);
	struct kn_string *string = *cache;

	if (KN_LIKELY(string != NULL)) {
		// cached strings must be allocated.
		assert(string->flags & KN_STRING_FL_STRUCT_ALLOC);
		assert(kn_string_length(string) == length);

		// if the string is the same, then that means we want the cached one.
		if (KN_LIKELY(strncmp(kn_string_deref(string), str, length) == 0))
			return kn_string_clone(string);

		evict_string(string);
	}

	// it may be embeddable, so don't just call `allocate_heap_string`.
	*cache = string = kn_string_alloc(length);
	string->flags |= KN_STRING_FL_CACHED;

	memcpy(kn_string_deref(string), str, length);
	kn_string_deref(string)[length] = '\0';

	return string;
}

void kn_string_deallocate(struct kn_string *string) {
	assert(string->refcount == 0);

	// If we're not cached, deallocate the string.
	if (!(string->flags & KN_STRING_FL_CACHED))
		deallocate_string(string);
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

	return kn_string_new_borrowed(
		kn_string_deref(string),
		kn_string_length(string)
	);
}

void kn_string_cleanup() {
	struct kn_string *string;

	for (unsigned i = 0; i < KN_STRING_CACHE_MAXLEN; ++i) {
		for (unsigned j = 0; j < KN_STRING_CACHE_LINELEN; ++j) {
			string = cache[i][j];

			if (string != NULL) {
				// we only cache allocated strings.
				assert(string->flags & KN_STRING_FL_STRUCT_ALLOC);

				// If there are no more references to it, deallocate the string.
				if (string->refcount == 0)
					deallocate_string(string);
			}
		}
	}
}


/*
 * Note that we can't use `strtoll`, as we can't be positive that `kn_number`
 * is actually a `long long`.
 */
kn_number kn_string_to_number(const struct kn_string *string) {
	kn_number ret = 0;
	const char *ptr = kn_string_deref(string);

	// strip leading whitespace.
	while (KN_UNLIKELY(isspace(*ptr)))
		ptr++;

	bool is_neg = *ptr == '-';

	// remove leading `-` or `+`s, if they exist.
	if (is_neg || *ptr == '+')
		++ptr;

	// only digits are `<= 9` when a literal `0` char is subtracted from them.
	unsigned char cur; // be explicit about wraparound.
	while ((cur = *ptr++ - '0') <= 9)
		ret = ret * 10 + cur;

	return is_neg ? -ret : ret;
}

struct kn_list *kn_string_to_list(const struct kn_string *string) {
	size_t length = kn_string_length(string);

	if (!length)
		return &kn_list_empty;

	struct kn_list *chars = kn_list_alloc(length);
	const char *ptr = kn_string_deref(string);

	for (unsigned i = 0; i < length; i++) {
		char buf[2] = { ptr[i], 0 };
		chars->elements[i] = kn_value_new_string(kn_string_new_borrowed(buf, 1));
	}

	return chars;
}
