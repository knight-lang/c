#ifndef KN_ENV_H
#define KN_ENV_H

#include "value.h"
#include "shared.h"
#include <stddef.h>

// it's declared within `env.c
struct kn_env;

/**
 * Initializes a Knight environment.
 *
 * This _must_ be called before `kn_env_fetch` is called.
 **/
struct kn_env *kn_env_create(size_t capacity_per_bucket, size_t number_of_buckets);

/**
 * Frees all resources associated with given Knight environment.
 *
 * This will invalidate all `kn_variable` pointers derived from this environment.
 **/
void kn_env_destroy(struct kn_env *env);

/**
 * A variable within Knight.
 *
 * This struct is only returned via `kn_env_fetch`, and lives for the remainder
 * of the program's lifetime. (Or, at least until `kn_env_free` is called.) As
 * such, there is no need to free it.
 **/
struct kn_variable {
	/*
	 * The value associated with this variable.
	 *
	 * When a variable is first fetched, this is set to `KN_UNDEFINED`, and
	 * should be overwritten before being used.
	 */
	kn_value value;

	/*
	 * The name of this variable.
	 */
	const char *name;
};

/**
 * Fetches the variable associated with the given identifier.
 *
 * This will always return a `kn_variable`, which may have been newly created.
 **/
struct kn_variable *kn_env_fetch(
	struct kn_env *env,
	const char *identifier,
	size_t length
);

/**
 * Assigns a value to this variable, overwriting whatever was there previously.
 **/
static inline void kn_variable_assign(
	struct kn_variable *variable,
	kn_value value
) {
	if (variable->value != KN_UNDEFINED)
		kn_value_free(variable->value);

	variable->value = value;
}

/**
 * Runs the given variable, returning the value associated with it.
 *
 * If the variable has not been assigned to yet, this will abort the program.
 **/
static inline kn_value kn_variable_run(struct kn_variable *variable) {
	if (KN_UNLIKELY(variable->value == KN_UNDEFINED))
		kn_error("undefined variable '%s'", variable->name);

	return kn_value_clone(variable->value);
}

static inline void kn_variable_dump(
	const struct kn_variable *variable,
	FILE *out
) {
	fprintf(out, "Variable(%s)", variable->name);
}

#endif /* !KN_ENV_H */
