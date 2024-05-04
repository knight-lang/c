#ifndef KN_VM_H
#define KN_VM_H

#include "opcode.h"
#include "types.h"

union kn_bytecode {
	enum kn_opcode opcode;
	unsigned int offset;
};

struct kn_vm {
	const kn_value *constants;
	struct kn_variable *variables;
	union kn_bytecode *bytecode;
	kn_value *stack;
	size_t stack_length, stack_cap;
#ifndef KN_NDEBUG
	size_t bytecode_length, variables_length, constants_length;
#endif
};

kn_value kn_vm_run(struct kn_vm *vm, size_t start);

#endif
