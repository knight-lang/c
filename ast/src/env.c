/**
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
 **/

#include <string.h>  /* strdup, strcmp */
#include <assert.h>  /* assert */
#include <stdlib.h>  /* free */
#include <stdbool.h> /* bool, true, false */
#include "env.h"     /* prototypes, size_t, kn_variable, kn_value, KN_UNDEFINED,
                        kn_value_free, kn_value_clone */
#include "shared.h"  /* kn_die, kn_heap_malloc, kn_heap_realloc, kn_hash, KN_UNLIKELY */

struct kn_env {
	size_t capacity_per_bucket, number_of_buckets;

	struct kn_env_bucket {
		size_t length;
		struct kn_variable *variables;
	} *buckets;
};

struct kn_env *kn_env_create(size_t capacity_per_bucket, size_t number_of_buckets) {
	struct kn_env *env = kn_heap_malloc(sizeof(struct kn_env));

	env->capacity_per_bucket = capacity_per_bucket;
	env->number_of_buckets = number_of_buckets;
	env->buckets = kn_heap_malloc(sizeof(struct kn_env_bucket) * number_of_buckets);

	for (size_t i = 0; i < number_of_buckets; ++i) {
		env->buckets[i].length = 0;
		env->buckets[i].variables = kn_heap_malloc(sizeof(struct kn_variable) * capacity_per_bucket);
	}

	return env;
}

void kn_env_destroy(struct kn_env *env) {
	for (size_t i = 0; i < env->number_of_buckets; ++i) {
		struct kn_env_bucket *bucket = &env->buckets[i];

		for (size_t len = 0; len < bucket->length; ++len) {
			// All identifiers are owned, and only marked `const` so
			// that users dont modify them (as it'd break the hash
			// function).
			KN_CLANG_IGNORE("-Wcast-qual",
				kn_heap_free((char *) bucket->variables[len].name);
			)

			// If the variable was defined in the source code, but
			// never assigned, it'll have a value of `KN_UNDEFINED`.
			if (bucket->variables[len].value != KN_UNDEFINED)
				kn_value_free(bucket->variables[len].value);
		}

		kn_heap_free(bucket->variables);
	}

	kn_heap_free(env);
}

struct kn_variable *kn_env_fetch(struct kn_env *env, const char *identifier, size_t length) {
	assert(length != 0);

	kn_hash_t hash = kn_hash(identifier, length);
	struct kn_env_bucket *bucket = &env->buckets[hash & (env->number_of_buckets - 1)];

	for (size_t i = 0; i < bucket->length; ++i) {
		struct kn_variable *variable = &bucket->variables[i];

		// If the variable already exists, return it.
		if (!strncmp(variable->name, identifier, length))
			return variable;
	}

	// If the bucket is full, then too many variables have been defined.
	if (KN_UNLIKELY(bucket->length == env->capacity_per_bucket))
		kn_die("too many variables created!");

	struct kn_variable *variable = &bucket->variables[bucket->length++];

	// Uninitialized variables start with an undefined starting value. The
	// new variable with an undefined starting value, so that any attempt to
	// access it will be invalid.
	variable->value = KN_UNDEFINED;
	variable->name = kn_memdup(identifier, length);

	return variable;
}
