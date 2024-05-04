#include <stdio.h>  /* vfprintf, fprintf, stderr */
#include <stdlib.h> /* exit, malloc, realloc */
#include <assert.h> /* assert */
#include <string.h>
#include "shared.h" /* prototypes, size_t, NULL, KN_UNLIKELY */
#include "allocator.h" /* prototypes, size_t, NULL, KN_UNLIKELY */

kn_hash_t kn_hash(const char *str, size_t length) {
	// start a `kn_hash_acc` with the default starting value
	return kn_hash_acc(str, length, 525201411107845655L);
}

kn_hash_t kn_hash_acc(const char *str, size_t length, kn_hash_t hash) {
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

void *kn_memdup(const void *mem, size_t length) {
	void *new = kn_heap_malloc(length);

	if (new)
		memcpy(new, mem, length);

	return new;
}
