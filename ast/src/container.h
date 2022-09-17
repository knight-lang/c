#ifndef KN_CONTAINER_H
#define KN_CONTAINER_H

#include "refcount.h"
#include <stddef.h>

struct kn_container {
	struct kn_refcount refcount;
	size_t length;
};

#define kn_length(ptr) (((struct kn_container *) (ptr))->length)
#define kn_length_set(ptr, value) (((struct kn_container *) (ptr))->length = (value))

#endif /* !KN_CONTAINER_H */
