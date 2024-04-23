#ifndef KN_ALLOCATOR_H
#define KN_ALLOCATOR_H

#include "shared.h"

#ifdef kn_gc
# undef kn_gc
# include "gc.h"
# define kn_allocation
#elif !defined(kn_leak)
# include "refcount.h"
# define kn_allocation struct kn_refcount refcount;
#else
# define kn_allocation
# ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wpedantic"
#  warning leaking memory
#  pragma message("leaking memory")
#  pragma GCC diagnostic pop
# endif
#endif

#include <stdint.h>

/**
 * Allocates `size` bytes of memory and returns a pointer to them.
 *
 * This is identical to the stdlib's `malloc`, except the program is aborted
 * instead of returning `NULL`.
 **/
void *
#if KN_HAS_ATTRIBUTE(malloc)
KN_ATTRIBUTE(malloc)
#endif
#if KN_HAS_ATTRIBUTE(returns_nonnull)
KN_ATTRIBUTE(returns_nonnull)
#endif
kn_heap_malloc(size_t size);

/**
 * Resizes the pointer to a segment of at least `size` bytes of memory and
 * returns the new segment's pointer.
 *
 * This is identical to the stdlib's `realloc`, except the program is aborted
 * instead of returning `NULL`.
 **/
void *
#if KN_HAS_ATTRIBUTE(returns_nonnull)
KN_ATTRIBUTE(returns_nonnull)
#endif
kn_heap_realloc(void *ptr, size_t size);

/**
 * Frees the memory at `ptr`
 */
static inline void kn_heap_free(void *ptr) {
	extern void free(void *);
	free(ptr);
}

#endif
