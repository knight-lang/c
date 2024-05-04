#include "alloc.h"

void *kn_heap_malloc(size_t size) {
	void *ptr = malloc(size);

#ifdef KN_RECKLESS
# if KN_HAS_ATTRIBUTE(__builtin_assume)
	__builtin_assume(ptr != NULL);
# endif /* __builtin_assume */
#else
	if (KN_UNLIKELY(ptr == NULL && size != 0)) {
		fprintf(stderr, "malloc failure for size %zu\n", size);
		abort();
	}
#endif /* KN_RECKLESS */

	return ptr;
}

void *kn_heap_realloc(void *ptr, size_t size) {
	void *newptr = realloc(ptr, size);

#ifdef KN_RECKLESS
# if KN_HAS_ATTRIBUTE(__builtin_assume)
	__builtin_assume(newptr != NULL || size == 0);
# endif /* __builtin_assume */
#else
	if (KN_UNLIKELY(newptr == NULL && size != 0)) {
		fprintf(stderr, "realloc failure for size %zu\n", size);
		abort();
	}
#endif /* KN_RECKLESS */

	return newptr;
}
