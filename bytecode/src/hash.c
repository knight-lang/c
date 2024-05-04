#include "hash.h"
#include "debug.h"

kn_hash_int kn_hash_acc(const char *str, size_t length, kn_hash_int hash) {
	kn_assert_nonnull(str);

	// This is MurmurHash.
	while (length--) {
		// make sure not EOS before `length` is over.
		kn_assert_ne(*str, '\0');

		hash ^= *str++;
		hash *= 0x5bd1e9955bd1e995;
		hash ^= hash >> 47;
	}

	return hash;
}
