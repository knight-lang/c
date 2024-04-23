#include "allocator.h"

void *kn_heap_malloc(size_t size) {
	void *ptr = malloc(size);

#ifndef KN_RECKLESS
	if (KN_UNLIKELY(ptr == NULL)) {
		fprintf(stderr, "malloc failure for size %zd\n", size);
		abort();
	}
#endif /* !KN_RECKLESS */

	return ptr;
}

void *kn_heap_realloc(void *ptr, size_t size) {
	ptr = realloc(ptr, size);

#ifndef KN_RECKLESS
	if (KN_UNLIKELY(ptr == NULL && size != 0)) {
		fprintf(stderr, "realloc failure for size %zd\n", size);
		abort();
	}
#endif /* !KN_RECKLESS */

	return ptr;
}
