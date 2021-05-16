#include "string.h" /* prototypes, kn_string, kn_string_flags variants, size_t,
                       KN_STRING_NEW_EMBED */
#include "shared.h" /* xmalloc, kn_hash, KN_LIKELY, KN_UNLIKELY */
#include <stdlib.h> /* free, NULL */
#include <string.h> /* strlen, strcmp, memcpy, strndup, strncmp, memcmp */
#include <assert.h> /* assert */

// The empty string.
// we need the alignment for embedding.
struct kn_string _Alignas(8) kn_string_empty = KN_STRING_NEW_EMBED("");

#ifndef KN_STRING_CACHE_MAXLEN
# define KN_STRING_CACHE_MAXLEN 32
#endif /* !KN_STRING_CACHE_MAXLEN */

#ifndef KN_STRING_CACHE_LINELEN
# define KN_STRING_CACHE_LINELEN (1<<14)
#endif /* !KN_STRING_CACHE_LINELEN */

static struct kn_string cache[KN_STRING_CACHE_MAXLEN][KN_STRING_CACHE_LINELEN];

#define IS_CACHE_COLD(string) ((string)->flags == 0)

static struct kn_string *cache_lookup(unsigned long hash, size_t length) {
	assert(length != 0);
	assert(length <= KN_STRING_CACHE_MAXLEN);

	return &cache[length - 1][hash & (KN_STRING_CACHE_LINELEN - 1)];
}

struct kn_string *kn_string_cache_lookup(unsigned long hash, size_t length) {
	if (length == 0 || KN_STRING_CACHE_MAXLEN < length)
		return NULL;

	struct kn_string *string = cache_lookup(hash, length);
	return NULL;
}

static struct kn_string *get_cache_slot(const char *str, size_t length) {
	return cache_lookup(kn_hash(str, length), length);
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

bool kn_string_equal(const struct kn_string *lhs, const struct kn_string *rhs) {
	if (lhs == rhs) // shortcut if they have the same pointer.
		return true;

	if (kn_string_length(lhs) != kn_string_length(rhs))
		return false;

	return !memcmp(
		kn_string_deref((struct kn_string *) lhs),
		kn_string_deref((struct kn_string *) rhs),
		kn_string_length(lhs)
	);
}

// Allocate a `kn_string` and populate it with the given `str`.
static struct kn_string *allocate_heap_string(char *str, size_t length) {
	assert(str != NULL);
	assert(length != 0); // zero length strings are `empty`.

	struct kn_string *string = xmalloc(sizeof(struct kn_string));

	string->flags = KN_STRING_FL_STRUCT_ALLOC;
	string->refcount = 1;
	string->alloc.length = length;
	string->alloc.str = str;

	return string;
}

static struct kn_string *allocate_embed_string(size_t length) {
	assert(length != 0);

	struct kn_string *string = xmalloc(sizeof(struct kn_string));

	string->flags = KN_STRING_FL_STRUCT_ALLOC | KN_STRING_FL_EMBED;
	string->refcount = 1;
	string->embed.length = length;

	return string;
}

// Actually deallocates the data associated with `string`.
static void deallocate_string(struct kn_string *string) {
	assert(string != NULL);
	assert(string->refcount == 0); // don't dealloc live strings...
	assert(string->flags);

	// If the struct isn't actually allocated, then return.
	if (!(string->flags & KN_STRING_FL_STRUCT_ALLOC)) {
		// Sanity check, as these are the only two non-struct-alloc flags.
		assert(string->flags & (KN_STRING_FL_EMBED | KN_STRING_FL_STATIC));
		return;
	}

	// If we're not embedded, free the allocated string
	if (KN_UNLIKELY(!(string->flags & KN_STRING_FL_EMBED)))
		free(string->alloc.str);

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

// Cache a string retuend by `kn_string_alloc`.
void kn_string_cache(struct kn_string **string) {
	// empty strings should never be cached.
	assert(kn_string_length(*string) != 0);
	assert((*string)->refcount == 1);

	// If it's too large for the cache, then just ignore it.
	if (KN_STRING_CACHE_MAXLEN < kn_string_length(*string))
		return;

	struct kn_string *slot = get_cache_slot(
		kn_string_deref(*string),
		kn_string_length(*string)
	);

	// would imply we're cahing a non-alloc result.
	assert(*string != slot);

	// If it's not cold.
	if (slot->flags) {
		// if there's something, then we don't cache.
		if (slot->refcount)
			return;

		// nothing there, so free what was there.
		if (!(slot->flags & KN_STRING_FL_EMBED))
			free(slot->alloc.str);
	}

	memcpy(slot, *string, sizeof(struct kn_string));
	*string = slot;
	slot->flags |= KN_STRING_FL_CACHED;
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

	struct kn_string *string = get_cache_slot(str, length);

	assert(string != NULL);

	// if we have no flags, it's entirely unused.
	if (!string->flags) 
		goto cold_string;

	assert(string->flags & KN_STRING_FL_CACHED);
	assert(kn_string_length(string) == length);

	// if they're the same string, we're good.
	if (!strncmp(kn_string_deref(string), str, length)) {
		free(str);
		++string->refcount;
		return string;
	}

	// if the string is currently populated, that's not good, make a new string.
	if (string->refcount)
		return allocate_heap_string(str, length);

	if (!(string->flags & KN_STRING_FL_EMBED))
		free(string->alloc.str);

cold_string:
	string->flags = KN_STRING_FL_CACHED;

	assert(string->refcount == 0);
	// otherwise, increment the refcount on the string, as we well reuse its buffer.
	string->refcount = 1;

	string->alloc.length = length;
	string->alloc.str = str;
	return string;
	// struct kn_string *string = get_cache_slot(str, length);
	// struct kn_string *string = *cacheline;

	// if (KN_LIKELY(string != NULL)) {
	// 	// if it's the same as `str`, use the cached version.
	// 	if (KN_LIKELY(strcmp(kn_string_deref(string), str) == 0)) {
	// 		;//fprintf(stderr, "hit\n");
	// 		free(str); // we don't need this string anymore, free it.
	// 		return kn_string_clone(string);
	// 	}
	// 		;//fprintf(stderr, "miss\n");

	// 	evict_string(string);
	// } else {
	// 		;//fprintf(stderr, "cold\n");
	// }

	// *cacheline = string = allocate_heap_string(str, length);
	// string->flags |= KN_STRING_FL_CACHED;
}

struct kn_string *kn_string_new_borrowed(const char *str, size_t length) {
	if (KN_UNLIKELY(length == 0))
		return &kn_string_empty;

	if (KN_STRING_CACHE_MAXLEN < length)
		return allocate_heap_string(strndup(str, length), length);

	struct kn_string *string = get_cache_slot(str, length);

	assert(string != NULL);

	// if we have no flags, it's entirely unused.
	if (!string->flags) 
		goto cold_string;

	assert(string->flags & KN_STRING_FL_CACHED);
	assert(kn_string_length(string) == length);

	// if they're the same string, we're good.
	if (!strncmp(kn_string_deref(string), str, length)) {
		++string->refcount;
		return string;
	}

	// if the string is currently populated, that's not good, make a new string.
	if (string->refcount) {
		string = kn_string_alloc(length);
	} else {
		// otherwise, increment the refcount on the string, as we well reuse its buffer.
		++string->refcount;
	}

populate:

	// populate the string--either the new one we allocated, or reuse the previous one.
	memcpy(kn_string_deref(string), str, length);
	kn_string_deref(string)[length] = '\0';
	return string;

	// the string wasn't populated 
cold_string:
	string->refcount = 1;
	string->flags = KN_STRING_FL_CACHED;

	// if we're embeddable, simply mark that
	if (length <= KN_STRING_EMBEDDED_LENGTH) {
		string->embed.length = length;
		string->flags |= KN_STRING_FL_EMBED;
	} else {
		// otherwise, we have to allocate the buffer.
		string->alloc.length = length;
		string->alloc.str = xmalloc(length + 1);
	}

	goto populate;

	// if it's currently occupied, tough.
	// if (string->refcount != 0)
	// 	return allocate_heap_string(strndup(str, length), length);

	// string->refcount = 1;

	// if (length <= KN_STRING_EMBEDDED_LENGTH) {
	// 	string->flags = KN_STRING_FL_STRUCT_ALLOC | KN_STRING_FL_EMBED;
	// string->refcount = 1;
	// string->embed.length = length;

	// // if it's entirely unoccupied, allocate it.
	// kn_string_alloc
	// string

	// if (KN_LIKELY(string != NULL)) {
	// 	// cached strings must be allocated.
	// 	assert(string->flags & KN_STRING_FL_STRUCT_ALLOC);
	// 	assert(kn_string_length(string) == length);

	// 	// if the string is the same, then that means we want the cached one.
	// 	if (KN_LIKELY(strncmp(kn_string_deref(string), str, length) == 0)) {
	// 		;//fprintf(stderr, "hit\n");
	// 		return kn_string_clone(string);
	// 	}

	// 	;//fprintf(stderr, "cold\n");
	// 	evict_string(string);
	// } else 
	// 	;//fprintf(stderr, "miss\n");

	// // it may be embeddable, so don't just call `allocate_heap_string`.
	// *cache = string = kn_string_alloc(length);
	// string->flags |= KN_STRING_FL_CACHED;

	// memcpy(kn_string_deref(string), str, length);
	// kn_string_deref(string)[length] = '\0';
}

void kn_string_free(struct kn_string *string) {
	assert(string != NULL);

	// If we're the last reference and not cached, deallocate the string.
	if (!--string->refcount && !(string->flags & KN_STRING_FL_CACHED))
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

	if (1) return;

	for (unsigned i = 0; i < KN_STRING_CACHE_MAXLEN; ++i) {
		for (unsigned j = 0; j < KN_STRING_CACHE_LINELEN; ++j) {
			string = &cache[i][j];

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

