#ifndef KN_CONTAINER_H
#define KN_CONTAINER_H

#include "refcount.h"
#include <stddef.h>

struct kn_container {
	struct kn_refcount refcount;
	size_t length;
};

#define kn_length(ptr) (((size_t *) (ptr))[1])

#endif /* !KN_CONTAINER_H */
