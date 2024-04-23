#ifndef KN_GC_H
#define KN_GC_H

#define kn_gc

// #include "program.h"
// #include "valuedecl.h"

void kn_gc_init(unsigned long long heap_size);
void kn_gc_start(void);
void kn_gc_teardown(void);
// enum kn_value_tag;
void *kn_gc_malloc(int/*enum kn_value_tag*/);

#endif 
