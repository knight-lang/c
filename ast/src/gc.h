#ifndef KN_GC_H
#define KN_GC_H

#define kn_gc

void kn_gc_init(unsigned long long heap_size);
void kn_gc_start(void);
void kn_gc_teardown(void);
void *kn_gc_malloc(void);

#endif 
