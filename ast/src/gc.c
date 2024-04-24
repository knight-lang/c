#include "gc.h"
#ifndef KN_USE_GC
struct _ignored;
#else
#include <stdlib.h>
#include "allocator.h"
#include "shared.h"
#include <sys/mman.h>

#define KN_VALUE_SIZE 64 // todo: replace with actual value
struct anyvalue {
	kn_value_header
	_Alignas(8) char _ignored[KN_VALUE_SIZE];
};

// KN_STATIC_ASSERT(sizeof(struct anyvalue) == KN_VALUE_SIZE, "size isnt equal");
static struct anyvalue *heap_start, *heap;
static unsigned long long heap_size;

void kn_gc_init(unsigned long long heap_size_) {
	heap_size = heap_size_ * KN_VALUE_SIZE;

	heap_start = heap = mmap(NULL, heap_size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, 0, 0);

	if (heap == MAP_FAILED)
		kn_die("unable to mmap %llu bytes for the heap", heap_size);

	// kn_log(gc, 1, "initialized gc heap with %lld (0x%llx) bytes of memory", heap_size, heap_size);
}

void kn_gc_teardown(void) {
	if (munmap(heap_start, heap_size))
		kn_die("unable to un-mmap %llu bytes for the heap", heap_size);
}

void *kn_gc_malloc_fn(void) {
	if (KN_UNLIKELY((heap_size / KN_VALUE_SIZE) <= (heap - heap_start))) {
		kn_gc_start();

		if (KN_UNLIKELY((heap_size / KN_VALUE_SIZE) <= heap - heap_start))
			kn_die("heap exhausted.");
	}

	while (heap->used) {
		// printf("skipping in-use address %p", (void *) heap);
		++heap;
	}
	heap->used = 1;
	// printf("found unused heap at address %p", (void *) heap);
	return heap++;
}

void kn_gc_start(void) {
/*#if KN_LOG_GC >= 1
#define INCR_METRIC(name) do { name++; } while(0);
	long unsigned unused = 0, marked = 0, freed = 0;
	kn_log(gc, 1, "starting gc cycle");
#else
#define INCR_METRIC(name) do { } while(0)
#endif

	kn_program_mark(program);

	for (struct anyvalue *ptr = heap_start; ptr < heap; ++ptr) {
		if (!ptr->basic.used) {
			kn_log(gc, 2, "ptr %p is in unused", heap);
			INCR_METRIC(unused);
			continue;
		}

		if (ptr->basic.marked) {
			kn_log(gc, 2, "ptr %p is in marked", heap);
			INCR_METRIC(marked);
			ptr->basic.marked = 0;
			continue;
		}

		kn_log(gc, 2, "ptr %p is in unmarked", heap);
		INCR_METRIC(freed);

		kn_value_deallocate(kn_value_new_ptr_unchecked((void *) ptr, ptr->basic.genus));
		ptr->basic.used = 0;
	}

	kn_log(gc, 1, "gc finished: %lu unused, %lu marked, %lu freed", unused, marked, freed);
	heap = heap_start;*/
}
#endif
