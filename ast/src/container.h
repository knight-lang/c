#ifndef KN_CONTAINER_H
#define KN_CONTAINER_H

#include "refcount.h"
#include <stddef.h>

/**
 * The header for all the allocated containers in knight---lists and strings.
 * 
 * By defining a standardized header, you're able to get the length of a value without having
 * to downcast a value to a list of string.
 **/
struct kn_container {
	/**
	 * All containers also have an associated refcount.
	 **/
	struct kn_refcount refcount;

	/**
	 * The length of the container.
	 **/
	size_t length;
};

/**
 * Gets the length of a container.
 **/
#define kn_length(ptr) (((struct kn_container *) (ptr))->length)

/**
 * Sets the length of a container.
 * 
 * The reason that `kn_length` doesn't return a pointer is because it's uncommon to actually set the
 * lengths of collections---since they're immutable it's only done at creation time. As such, it's
 * a separate macro to avoid potential changing of lengths without intending it.
 **/
#define kn_length_set(ptr, value) (((struct kn_container *) (ptr))->length = (value))

#endif /* !KN_CONTAINER_H */
