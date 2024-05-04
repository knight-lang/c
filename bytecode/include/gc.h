#ifndef KN_GC_H
#define KN_GC_H

#define KN_VALUE_ALIGNMENT 8

#define KN_HEADER _Alignas(KN_VALUE_ALIGNMENT) unsigned char flags;

#define kn_flags(ptr) KN_CLANG_IGNORE("-Wcast-qual", (((unsigned char *)(ptr))[offsetof(struct { KN_HEADER }, flags)]))
#define KN_GC_FL_MARKED (1 << ((8 * sizeof(unsigned int)) - 1))

#endif
