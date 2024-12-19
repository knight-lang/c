#ifndef KN_VM_H
#define KN_VM_H

#include "opcode.h"
#include "types.h"

struct kn_vm {
	size_t ip, stack_len, stack_cap;
	kn_value *variables, *stack;
	const struct kn_code *code;
};

kn_value kn_vm_run(struct kn_vm *vm, size_t start);

#endif
