#ifndef KN_ALLOCATOR_H
#define KN_ALLOCATOR_H

#include "shared.h"
#include <stdalign.h>
// #define KN_USE_GC

#if defined(KN_USE_REFCOUNT) && defined(KN_USE_GC)
# error cant use both gc and refcount
#endif

#define KN_VALUE_ALIGNMENT 8

#ifdef KN_USE_REFCOUNT
# define KN_HEADER alignas(KN_VALUE_ALIGNMENT) size_t refcount; \
                   unsigned char flags;
# define kn_refcount(ptr) (*(size_t*) (((unsigned char *)(ptr) + offsetof(struct { KN_HEADER }, refcount))))
#else
# define KN_HEADER alignas(KN_VALUE_ALIGNMENT) unsigned char flags;
#endif /* KN_USE_REFCOUNT */

#define kn_flags(ptr) (((unsigned char *)(ptr))[offsetof(struct { KN_HEADER }, flags)])

#ifdef KN_USE_GC
# include "gc.h"
# define KN_GC_FL_MARKED (1 << ((8 * sizeof(unsigned int)) - 1))
#endif


#ifndef KN_HEADER
// # message oops
// # ifdef __GNUC__
// #  pragma GCC diagnostic push
// #  pragma GCC diagnostic ignored "-Wpedantic"
// // #  warning leaking memory
// // #  pragma message("leaking memory")
// #  pragma GCC diagnostic pop
// # endif
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
#if KN_HAS_ATTRIBUTE(alloc_size)
 KN_ATTRIBUTE(alloc_size(1))
#endif
#if KN_HAS_ATTRIBUTE(warn_unused_result)
 KN_ATTRIBUTE(warn_unused_result)
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
#if KN_HAS_ATTRIBUTE(warn_unused_result)
 KN_ATTRIBUTE(warn_unused_result)
#endif
#if KN_HAS_ATTRIBUTE(alloc_size)
 KN_ATTRIBUTE(alloc_size(2))
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
