#ifndef KN_ALLOCATOR_H
#define KN_ALLOCATOR_H

#ifdef kn_gc
# undef kn_gc
# include "gc.h"
# define kn_allocation
#elif !defined(kn_leak)
# include "refcount.h"
# define kn_allocation struct kn_refcount refcount;
#endif

#endif
