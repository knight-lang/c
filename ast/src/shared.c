#include <stdio.h>  /* vfprintf, fprintf, stderr */
#include <stdlib.h> /* exit, malloc, realloc */
#include <assert.h> /* assert */
#include "shared.h" /* prototypes, size_t, NULL, KN_UNLIKELY */

unsigned long kn_hash_acc(const char *str, size_t length, unsigned long hash) {
	assert(str != NULL);

	// This is the MurmurHash.
	while (length--) {
		assert(*str != '\0'); // make sure not EOS before `length` is over.

		hash ^= *str++;
		hash *= 0x5bd1e9955bd1e995;
		hash ^= hash >> 47;
	}

	return hash;
}

unsigned long kn_hash(const char *str, size_t length) {
	// start a `kn_hash_acc` with the default starting value
	return kn_hash_acc(str, length, 525201411107845655L);
}

void *xmalloc(size_t size) {
	void *ptr = malloc(size);

#ifndef KN_RECKLESS
	if (KN_UNLIKELY(ptr == NULL)) {
		fprintf(stderr, "malloc failure for size %zd\n", size);
		abort();
	}
#endif /* !KN_RECKLESS */

	return ptr;
}

void *xrealloc(void *ptr, size_t size) {
	ptr = realloc(ptr, size);

#ifndef KN_RECKLESS
	if (KN_UNLIKELY(ptr == NULL && size != NULL)) {
		fprintf(stderr, "realloc failure for size %zd\n", size);
		abort();
	}
#endif /* !KN_RECKLESS */

	return ptr;
}
