#ifndef KN_HASH_H
#define KN_HASH_H

#include <stddef.h>

/* The type used for hashing */
typedef unsigned long long kn_hash_int;

/**
 * Computes the hash of the first `length` characters of `str`, with the given
 * starting `hash`.
 *
 * This is useful to compute hashes of non-sequential strings.
 **/
kn_hash_int kn_hash_acc(const char *str, size_t length, kn_hash_int hash);

/**
 * Returns a hash for the first `length` characters of `str`.
 *
 * `str` must be at least `length` characters long, excluding any trailing `\0`
 **/
static inline kn_hash_int kn_hash(const char *str, size_t length) {
	// start a `kn_hash_acc` with the default starting value
	return kn_hash_acc(str, length, 525201411107845655L);
}

#endif
