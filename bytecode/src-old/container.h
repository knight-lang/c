#ifndef KN_CONTAINER_H
#define KN_CONTAINER_H

#include "allocator.h"

#include <stdalign.h>
#include <stddef.h>

/**
 * The header for all the allocated containers in knight---lists and strings.
 * 
 * By defining a standardized header, you're able to get the length of a value without having
 * to downcast a value to a list of string.
 **/
#define KN_CONTAINER \
	KN_HEADER \
	size_t length;

#define kn_length(x) (x)->length
// struct kn_container {
// 	/**
// 	 * All containers also have an associated refcount.
// 	 **/
// 	KN_HEADER

// 	/**
// 	 * The length of the container.
// 	 **/
// 	size_t length;
// };

// /**
//  * The length of a container, which can be modified.
//  **/
// #define kn_length(ptr) KN_CLANG_IGNORE("-Wcast-qual", (((struct kn_container *) (ptr))->length))

#endif /* !KN_CONTAINER_H */
