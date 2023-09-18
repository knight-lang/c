#include "string.h" /* prototypes, kn_string, kn_string_flags variants, size_t,
                       KN_STRING_NEW_EMBED */
#include "shared.h" /* xmalloc, kn_hash, KN_LIKELY, KN_UNLIKELY */
#include <stdlib.h> /* free, NULL */
#include <string.h> /* strlen, strcmp, memcpy, strndup, strncmp, memcmp */
#include <assert.h> /* assert */
#include <ctype.h>
#include "list.h"

// The empty string.
// we need the alignment for embedding.
struct kn_string kn_string_empty = KN_STRING_NEW_EMBED("");

#ifdef KN_STRING_CACHE
# ifndef KN_STRING_CACHE_MAXLEN
#  define KN_STRING_CACHE_MAXLEN 32
# endif /* !KN_STRING_CACHE_MAXLEN */
# ifndef KN_STRING_CACHE_LINELEN
#  define KN_STRING_CACHE_LINELEN (1 << 14)
# endif /* !KN_STRING_CACHE_LINELEN */

static struct kn_string *cache[KN_STRING_CACHE_MAXLEN][KN_STRING_CACHE_LINELEN];

static struct kn_string **cache_lookup(kn_hash_t hash, size_t length) {
	assert(length != 0);
	assert(length <= KN_STRING_CACHE_MAXLEN);

	return &cache[length - 1][hash & (KN_STRING_CACHE_LINELEN - 1)];
}

struct kn_string *kn_string_cache_lookup(kn_hash_t hash, size_t length) {
	if (length == 0 || KN_STRING_CACHE_MAXLEN < length)
		return NULL;

	return *cache_lookup(hash, length);
}

static struct kn_string **get_cache_slot(const char *str, size_t length) {
	return cache_lookup(kn_hash(str, length), length);
}
#endif /* KN_STRING_CACHE */

bool kn_string_equal(const struct kn_string *lhs, const struct kn_string *rhs) {
	if (lhs == rhs) // shortcut if they have the same pointer.
		return true;

	if (kn_length(lhs) != kn_length(rhs))
		return false;

	return !memcmp(kn_string_deref(lhs), kn_string_deref(rhs), kn_length(lhs));
}

kn_integer kn_string_compare(
	const struct kn_string *lhs,
	const struct kn_string *rhs
) {
	if (lhs == rhs)
		return 0;

	// FIXME: don't use strcmp, use memcmp
   return strcmp(kn_string_deref(lhs), kn_string_deref(rhs));
}


// Allocate a `kn_string` and populate it with the given `str`.
static struct kn_string *allocate_heap_string(char *str, size_t length) {
	assert(str != NULL);
	assert(length != 0); // zero length strings are `empty`.

	struct kn_string *string = xmalloc(sizeof(struct kn_string));

	string->flags = KN_STRING_FL_STRUCT_ALLOC;
	string->container = (struct kn_container) {
		.refcount = { 1 },
		.length = length
	};
	string->ptr = str;

	return string;
}

static struct kn_string *allocate_embed_string(size_t length) {
	assert(length != 0);

	struct kn_string *string = xmalloc(sizeof(struct kn_string));

	string->flags = KN_STRING_FL_STRUCT_ALLOC | KN_STRING_FL_EMBED;
	string->container = (struct kn_container) {
		.refcount = { 1 },
		.length = length
	};

	return string;
}

// Actually deallocates the data associated with `string`.
static void deallocate_string(struct kn_string *string) {
	assert(string != NULL);
	assert(kn_refcount(string) == 0); // don't dealloc live strings...

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

#ifdef KN_STRING_CACHE
static void evict_string_active(struct kn_string *string) {
	assert(kn_refcount(string) != 0);
	assert(string->flags & KN_STRING_FL_CACHED);

	string->flags -= KN_STRING_FL_CACHED;
}

static void evict_string(struct kn_string *string) {
	// we only cache allocated strings.
	assert(string->flags & KN_STRING_FL_STRUCT_ALLOC);

	if (kn_refcount(string) == 0) {
		// If there are no more references to it, deallocate the string.
		deallocate_string(string);
	} else {
		// otherwise, just evict it from the cache slot.
		evict_string_active(string);
	}
}
#endif /* KN_STRING_CACHE */

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
	assert(kn_length(string) != 0);

#ifdef KN_STRING_CACHE
	// If it's too large for the cache, then just ignore it.
	if (KN_STRING_CACHE_MAXLEN < kn_length(string))
		return;

	struct kn_string **cacheline = get_cache_slot(
		kn_string_deref(string),
		kn_length(string)
	);

	// If there was something there and has no live references left, free it.
	if (KN_LIKELY(*cacheline != NULL))
		evict_string(*cacheline);

	// Indicate that the string is now cached, and replace the old cache line.
	string->flags |= KN_STRING_FL_CACHED;
	*cacheline = string;
#else
	(void) string;
#endif /* KN_STRING_CACHE */

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

	struct kn_string *string;

#ifdef KN_STRING_CACHE
	// if it's too big just dont cache it
	if (KN_STRING_CACHE_MAXLEN < length)
		return allocate_heap_string(str, length);

	struct kn_string **cacheline = get_cache_slot(str, length);
	string = *cacheline;

	if (KN_LIKELY(string != NULL)) {
		// if it's the same as `str`, use the cached version.
		if (KN_LIKELY(strcmp(kn_string_deref(string), str) == 0)) {
			free(str); // we don't need this string anymore, free it.
			return kn_string_clone(string);
		}

		evict_string(string);
	}
#endif /* KN_STRING_CACHE */

	string = allocate_heap_string(str, length);
	string->flags |= KN_STRING_FL_CACHED;

#ifdef KN_STRING_CACHE
	*cacheline = string;
#endif /* KN_STRING_CACHE */

	return string;
}

struct kn_string *kn_string_new_borrowed(const char *str, size_t length) {
	if (KN_UNLIKELY(length == 0))
		return &kn_string_empty;

	struct kn_string *string;

#ifdef KN_STRING_CACHE
	if (KN_STRING_CACHE_MAXLEN < length)
		return allocate_heap_string(strndup(str, length), length);

	struct kn_string **cached = get_cache_slot(str, length);
	string = *cached;

	if (KN_LIKELY(string != NULL)) {
		// cached strings must be allocated.
		assert(string->flags & KN_STRING_FL_STRUCT_ALLOC);
		assert(kn_length(string) == length);

		// if the string is the same, then that means we want the cached one.
		if (KN_LIKELY(strncmp(kn_string_deref(string), str, length) == 0))
			return kn_string_clone(string);

		evict_string(string);
	}
#endif /* KN_STRING_CACHE */

	// it may be embeddable, so don't just call `allocate_heap_string`.
	string = kn_string_alloc(length);
	string->flags |= KN_STRING_FL_CACHED;

	memcpy(kn_string_deref(string), str, length);
	kn_string_deref(string)[length] = '\0';

#ifdef KN_STRING_CACHE
	*cached = string;
#endif /* KN_STRING_CACHE */

	return string;
}

void kn_string_dealloc(struct kn_string *string) {
	assert(kn_refcount(string) == 0);

	// If we're not cached, deallocate the string.
	if (!(string->flags & KN_STRING_FL_CACHED))
		deallocate_string(string);
}

struct kn_string *kn_string_clone_static(struct kn_string *string) {
	if (!(string->flags & KN_STRING_FL_STATIC))
		return string;

	return kn_string_new_borrowed(kn_string_deref(string), kn_length(string));
}

void kn_string_cleanup() {
#ifdef KN_STRING_CACHE
	struct kn_string *string;

	for (size_t i = 0; i < KN_STRING_CACHE_MAXLEN; ++i) {
		for (size_t j = 0; j < KN_STRING_CACHE_LINELEN; ++j) {
			string = cache[i][j];

			if (string != NULL) {
				// we only cache allocated strings.
				assert(string->flags & KN_STRING_FL_STRUCT_ALLOC);

				// If there are no more references to it, deallocate the string.
				if (kn_refcount(string) == 0)
					deallocate_string(string);
			}
		}
	}
#endif /* KN_STRING_CACHE */
}

struct kn_list *kn_string_to_list(const struct kn_string *string) {
	size_t length = kn_length(string);

	if (length == 0)
		return &kn_list_empty;

	struct kn_list *chars = kn_list_alloc(length);
	const char *ptr = kn_string_deref(string);

	for (size_t i = 0; i < length; i++) {
		char buf[2] = { ptr[i], 0 };
		kn_list_set(chars, i, kn_value_new(kn_string_new_borrowed(buf, 1)));
	}

	return chars;
}

struct kn_string *kn_string_concat(
	struct kn_string *lhs, 
	struct kn_string *rhs
) {
	size_t lhslen, rhslen;

	// return early if either
	if ((lhslen = kn_length(lhs)) == 0) {
		assert(lhs == &kn_string_empty);
		return kn_string_clone_static(rhs);
	}

	if ((rhslen = kn_length(rhs)) == 0) {
		assert(rhs == &kn_string_empty);
		return lhs;
	}

	kn_hash_t hash = kn_hash(kn_string_deref(lhs), lhslen);
	hash = kn_hash_acc(kn_string_deref(rhs), rhslen, hash);

	size_t length = lhslen + rhslen;

	struct kn_string *string;

#ifdef KN_STRING_CACHE
	string = kn_string_cache_lookup(hash, length);
	if (string == NULL)
		goto allocate_and_cache;

	char *cached = kn_string_deref(string);
	char *tmp = kn_string_deref(lhs);

	// FIXME: use memcmp instead
	for (size_t i = 0; i < lhslen; i++, cached++)
		if (*cached != tmp[i])
			goto allocate_and_cache;

	tmp = kn_string_deref(rhs);

	for (size_t i = 0; i < rhslen; i++, cached++)
		if (*cached != tmp[i])
			goto allocate_and_cache;

	string = kn_string_clone(string);
	goto free_and_return;

allocate_and_cache:
#endif /* KN_STRING_CACHE */

	string = kn_string_alloc(length);
	char *str = kn_string_deref(string);

	memcpy(str, kn_string_deref(lhs), lhslen);
	memcpy(str + lhslen, kn_string_deref(rhs), rhslen);
	str[length] = '\0';

	kn_string_cache(string);

#ifdef KN_STRING_CACHE
free_and_return:
#endif /* KN_STRING_CACHE */

	kn_string_free(lhs);
	kn_string_free(rhs);

	return string;
}

struct kn_string *kn_string_repeat(struct kn_string *string, size_t amount) {
	size_t lhslen = kn_length(string);

	if (lhslen == 0 || amount == 0) {
		// if the string is not empty, free it.
		if (lhslen == 0) {
			assert(string == &kn_string_empty);
		} else {
			kn_string_free(string);
		}

		return &kn_string_empty;
	}

	// we don't have to clone it, as we were given the cloned copy.
	if (amount == 1)
		return string;

	size_t length = lhslen * amount;
	struct kn_string *repeat = kn_string_alloc(length);
	char *str = kn_string_deref(repeat);

	for (char *ptr = str; amount != 0; --amount, ptr += lhslen)
		memcpy(ptr, kn_string_deref(string), lhslen);

	str[length] = '\0';

	kn_string_free(string);

	return repeat;
}

struct kn_string *kn_string_get_substring(
	struct kn_string *string,
	size_t start,
	size_t length
) {
	assert(start + length <= kn_length(string));

	if (length == 0) {
		assert(string == &kn_string_empty);
		return &kn_string_empty;
	}

	if (KN_UNLIKELY(!start && length == kn_length(string)))
		return kn_string_clone_static(string);

	struct kn_string *substring = kn_string_new_borrowed(
		kn_string_deref(string) + start,
		length
	);
	kn_string_free(string);
	return substring;
}

struct kn_string *kn_string_set_substring(
	struct kn_string *string,
	size_t start,
	size_t length,
	struct kn_string *replacement
) {
	assert(start + length <= kn_length(string));

	if (kn_length(replacement) == 0 && start == 0) {
		assert(replacement == &kn_string_empty);
		return kn_string_get_substring(string, length, kn_length(string) - length);
	}

	if (KN_UNLIKELY(kn_length(string) == 0)) {
		assert(string == &kn_string_empty);
		return kn_string_clone_static(replacement);
	}

	char *string_str = kn_string_deref(string);
	char *repl_str = kn_string_deref(replacement);

	size_t replaced_length = kn_length(string) - length + kn_length(replacement);
	kn_hash_t hash = kn_hash(string_str, start);
	hash = kn_hash_acc(repl_str, kn_length(replacement), hash);
	hash = kn_hash_acc(string_str + start + length, kn_length(string) - start - length, hash);

	struct kn_string *cached;

#ifdef KN_STRING_CACHE
	cached = kn_string_cache_lookup(hash, replaced_length);
	if (
		cached
		&& !memcmp(string_str, kn_string_deref(cached), start)
		&& !memcmp(repl_str, kn_string_deref(cached) + start, kn_length(replacement))
		&& !memcmp(
			string_str + start + length,
			kn_string_deref(cached) + start + kn_length(replacement),
			kn_length(string) - start - length
		)
	) {
		cached = kn_string_clone(cached);
		goto free_and_return;
	}
#endif /* KN_STRING_CACHE */

	cached = kn_string_alloc(replaced_length);
	char *str = kn_string_deref(cached);

	memcpy(str, string_str, start);
	memcpy(str + start, repl_str, kn_length(replacement));
	memcpy(str + start + kn_length(replacement), string_str + start + length, kn_length(string));
	str[replaced_length] = '\0';

	kn_string_cache(cached);

#ifdef KN_STRING_CACHE
free_and_return:
#endif /* KN_STRING_CACHE */

	kn_string_free(string);
	kn_string_free(replacement);

	return cached;
}

void kn_string_dump(const struct kn_string *string, FILE *out) {
	fputc('"', out);

	const char *ptr = kn_string_deref(string);
	for (size_t i = 0; i < kn_length(string); i++) {
		switch (ptr[i]) {
		case '\r':
			fputs("\\r", out);
			break;

		case '\n':
			fputs("\\n", out);
			break;

		case '\t':
			fputs("\\t", out);
			break;

		case '\\':
		case '\"':
			fputc('\\', out);
			KN_FALLTHROUGH

		default:
			fputc(ptr[i], out);
		}
	}

	fputc('"', out);
}
