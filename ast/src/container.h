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
 * The length of a container, which can be modified.
 **/
#define kn_length(ptr) (((struct kn_container *) (ptr))->length)

#endif /* !KN_CONTAINER_H */
