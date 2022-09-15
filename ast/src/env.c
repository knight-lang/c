/*
 * The environment of Knight is used to keep track of all the variables
 * within it.
 *
 * Instead of a naive approach of simply having variables keep track of their
 * names, and then looking up the corresponding variable on each execution, we
 * use the fact that all variables are globally allocated. As such, whenever
 * an unknown identifier is given to `kn_env_fetch`, we actually make a new
 * entry for it and assign it the undefined value `KN_UNDEFINED`.
 *
 * Therefore, whenever a variable is referenced in the source code, it's given
 * an entry, even if it's never actually assigned.
 */

#include <string.h>  /* strdup, strcmp */
#include <assert.h>  /* assert */
#include <stdlib.h>  /* free */
#include <stdbool.h> /* bool, true, false */
#include "env.h"     /* prototypes, size_t, kn_variable, kn_value, KN_UNDEFINED,
                        kn_value_free, kn_value_clone */
#include "shared.h"  /* die, xmalloc, xrealloc, kn_hash, KN_UNLIKELY */

/*
 * The amount of buckets that the `kn_env_map` will have.
 *
 * The greater the number, the fewer cache collisions, but the more memory used.
 */
#ifndef KN_ENV_NBUCKETS
# define KN_ENV_NBUCKETS 65536
#endif /* !KN_ENV_NBUCKETS */

/*
 * The capacity of each bucket.
 *
 * Once this many variables are in a single bucket, the program will have to
 * reallocate those buckets.
 */
#ifndef KN_ENV_CAPACITY
# define KN_ENV_CAPACITY 256
#endif /* !KN_ENV_CAPACITY */

#if KN_ENV_CAPACITY == 0
# error env capacity must be at least 1
#endif /* KN_ENV_CAPACITY == 0 */

/*
 * The buckets of the environment hashmap.
 *
 * Each bucket keeps track of its own individual capacity and length, so they
 * can be resized separately.
 */
struct kn_env_bucket {
	size_t capacity, length;
	struct kn_variable *variables;
};

/* The mapping of all variables within Knight. */
static struct kn_env_bucket kn_env_map[KN_ENV_NBUCKETS];

#ifndef NDEBUG
/* A sanity check to ensure the environment has been setup. */
static bool kn_env_has_been_started = false;
#endif /* !NDEBUG */

void kn_env_startup(void) {
	// make sure we haven't started, and then set started to true.
	assert(!kn_env_has_been_started && (kn_env_has_been_started = true));

	for (size_t i = 0; i < KN_ENV_NBUCKETS; ++i) {
		// Since it's static, `length` starts off at `0`, and
		// `kn_env_shutdown` should reset it back to zero.
		assert(kn_env_map[i].length == 0);
		kn_env_map[i].capacity = KN_ENV_CAPACITY;
		kn_env_map[i].variables = xmalloc(
			sizeof(struct kn_variable) * KN_ENV_CAPACITY
		);
	}
}

void kn_env_shutdown(void) {
	// make sure we've started, and then indicate we've shut down.
	assert(kn_env_has_been_started && !(kn_env_has_been_started = false));

	for (size_t i = 0; i < KN_ENV_NBUCKETS; ++i) {
		struct kn_env_bucket *bucket = &kn_env_map[i];

		for (size_t len = 0; len < bucket->length; ++len) {
			// All identifiers are owned, and only marked `const` so
			// that users dont modify them (as it'd break the hash
			// function).
			free((char *) bucket->variables[len].name);

			// If the variable was defined in the source code, but
			// never assigned, it'll have a value of `KN_UNDEFINED`.
			if (bucket->variables[len].value != KN_UNDEFINED)
				kn_value_free(bucket->variables[len].value);
		}

		free(bucket->variables);
		bucket->length = 0;
		bucket->capacity = 0;
	}
}

struct kn_variable *kn_env_fetch(const char *identifier, size_t length) {
	assert(length != 0);

	kn_hash_t hash = kn_hash(identifier, length);
	struct kn_env_bucket *bucket = &kn_env_map[hash & (KN_ENV_NBUCKETS - 1)];

	for (size_t i = 0; i < bucket->length; ++i) {
		struct kn_variable *variable = &bucket->variables[i];

		// If the variable already exists, return it.
		if (!strncmp(variable->name, identifier, length))
			return variable;
	}

	// If the bucket is full, then too many variables have been defined.
	if (KN_UNLIKELY(bucket->length == bucket->capacity))
		die("too many variables created!");

	struct kn_variable *variable = &bucket->variables[bucket->length++];

	// Uninitialized variables start with an undefined starting value. The
	// new variable with an undefined starting value, so that any attempt to
	// access it will be invalid.
	variable->value = KN_UNDEFINED;
	variable->name = strndup(identifier, length);

	return variable;
}
