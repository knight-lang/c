#include <stdio.h>  /* vfprintf, fprintf, stderr */
#include <stdlib.h> /* exit, malloc, realloc */
#include <assert.h> /* assert */
#include "shared.h" /* prototypes, size_t, ssize_t, NULL, KN_UNLIKELY */


unsigned long kn_hash(const char *str) {
	unsigned long hash;

	assert(str != NULL);

	// This is the MurmurHash.
	hash = 525201411107845655;

	while (*str != '\0') {
		hash ^= *str++;
		hash *= 0x5bd1e9955bd1e995;
		hash ^= hash >> 47;
	}

	return hash;
}


unsigned long kn_hashn(const char *str, size_t length) {
	unsigned long hash;

	assert(str != NULL);

	// This is the MurmurHash.
	hash = 525201411107845655;

	while (length--) {
		assert(*str != '\0'); // make sure not eos before `length` is over.
		hash ^= *str++;
		hash *= 0x5bd1e9955bd1e995;
		hash ^= hash >> 47;
	}

	return hash;
}

void *xmalloc(size_t size) {
	// printf("allocate: %zu\n", size);
	assert(0 <= (ssize_t) size);

	void *ptr = malloc(size);

#ifndef KN_RECKLESS
	if (KN_UNLIKELY(ptr == NULL)) {
		fprintf(stderr, "malloc failure for size %zd", size);
		abort();
	}
#endif /* !KN_RECKLESS */

	return ptr;
}

void *xrealloc(void *ptr, size_t size) {
	assert(0 <= (ssize_t) size);

	ptr = realloc(ptr, size);

#ifndef KN_RECKLESS
	if (KN_UNLIKELY(ptr == NULL)) {
		die("realloc failure for size %zd", size);
		abort();
	}
#endif /* !KN_RECKLESS */

	return ptr;
}
