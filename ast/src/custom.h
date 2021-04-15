#if defined(KN_CUSTOM) && !defined(KN_CUSTOM_H)
#define KN_CUSTOM_H

#include "value.h"  /* kn_value, kn_number, kn_boolean, kn_string */
#include <stddef.h> /* size_t */

/*
 * The virtual table for custom types.
 *
 * There should generally only be one of these per type. Functions may be
 * omitted (by assigning them to `NULL`); if they are, their default
 * implementations will be used.
 *
 * However, at a minimum, either `run` or all of `to_number`, `to_boolean`, and
 * `to_string` must be defined.
 *
 * Note that the `data` value passed to each function is actually the pointer to
 * `kn_custom`'s `data` field. As such, the original `custom` can be recovered
 * if needed.
 */
struct kn_custom_vtable {
	/*
	 * Releases the resources associated with `data`.
	 *
	 * Note that this should not `data` itself, but only resources associated
	 * with it. The default implementation does nothing.
	 */
	void (*free)(void *data);

	/*
	 * Dumps debugging info for `data` without a trailing newline.
	 *
	 * The default implementation writes `Custom(<data ptr>, <vtable ptr>)` to
	 * stdout.
	 */
	void (*dump)(void *data);

	/*
	 * Executes the given `data`, returning the value associated with it.
	 *
	 * The default implementation simply returns the custom itself.
	 */
	kn_value (*run)(void *data);

	/*
	 * Converts the `data` to a number.
	 *
	 * The default implementation will call `run`, and then convert the result
	 * to a number.
	 */
	kn_number (*to_number)(void *data);

	/*
	 * Converts the `data` to a boolean.
	 *
	 * The default implementation will call `run`, and then convert the result
	 * to a boolean.
	 */
	kn_boolean (*to_boolean)(void *data);

	/*
	 * Converts the `data` to a string.
	 *
	 * The default implementation will call `run`, and then convert the result
	 * to a string.
	 */
	struct kn_string *(*to_string)(void *data);
};

/*
 * The struct that represents user-defined data within Knight.
 */
struct kn_custom {
	/*
	 * The vtable associated with this struct.
	 *
	 * This will never be freed, and should probably be a global value.
	 */
	const struct kn_custom_vtable *vtable;

	/*
	 * The reference count for this type.
	 *
	 * This is manipulated via `kn_custom_free` and `kn_custom_clone`.
	 */
	unsigned refcount;

	/*
	 * The custom data for this struct.
	 *
	 * This field is passed to all the function within the vtable.
	 */
	// IMPLEMENTATION NOTE: `_Alignas(max_align_t)` is required because `data`
	// can hold any type, and thus must have the alignment of any type.
	_Alignas(max_align_t) char data[];
};

/*
 * Returns a pointer to a new `kn_custom`, whose `data` field can hold at least
 * `size` bytes.
 *
 * The returned `kn_custom` must be passed to `kn_custom_free` when it is no
 * longer in use to prevent resource leaks.
 *
 * Note that `vtable` may not be NULL, and should point to a valid vtable.
 */
struct kn_custom *kn_custom_alloc(
	size_t size,
	const struct kn_custom_vtable *vtable
);

/*
 * Indicates that this reference to `custom` is no longer needed.
 *
 * If called with the last reference to the custom struct, this will free the
 * resources associated with the struct. If a `free` function is provided in the
 * constructor, it will also be called.
 */
void kn_custom_free(struct kn_custom *custom);

/*
 * Returns a new reference to `custom`.
 *
 * After use, both `custom` and the returned struct must be passed to
 * `kn_custom_free` to ensure that resources are cleaned up.
 */
struct kn_custom *kn_custom_clone(struct kn_custom *custom);

#endif /* KN_CUSTOM && !KN_CUSTOM_H */
