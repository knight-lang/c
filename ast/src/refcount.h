#ifndef KN_REFCOUNT_H
#define KN_REFCOUNT_H

#include <stdalign.h>
#include <stddef.h>

struct kn_refcount {
	alignas(8) size_t count;
};

#define kn_refcount(ptr) (&((struct kn_refcount *) (ptr))->count)

#endif /* !KN_REFCOUNT_H */
