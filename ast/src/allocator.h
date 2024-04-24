#ifndef KN_ALLOCATOR_H
#define KN_ALLOCATOR_H

#include "shared.h"
// #define KN_USE_GC

#ifdef KN_USE_GC
# include "gc.h"
# define kn_value_header unsigned long long used;
#endif

#ifdef KN_USE_REFCOUNT
# ifdef KN_USE_GC
#  error cant use both gc and refcount
# endif
# include "refcount.h"
# define kn_value_header struct kn_refcount refcount;
# define KN_IF_RC(...) __VA_ARGS__
#else
# define KN_IF_RC(...)
#endif

#ifndef kn_value_header
# define kn_value_header
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