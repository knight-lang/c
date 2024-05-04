#include "vm.h"
#include "alloc.h"
#include "value.h"

struct kn_stackframe {
	const struct kn_vm *vm;
	size_t ip;
};

static void push(struct kn_vm *vm, kn_value value) {
	if (KN_UNLIKELY(vm->stack_length == vm->stack_cap))
		vm->stack = kn_heap_realloc(vm->stack, vm->stack_cap *= 2);
	vm->stack[vm->stack_length++] = value;
}

static kn_value pop(struct kn_vm *vm) {
	kn_assertm(vm->stack_length != 0, "bug, vm stack popped when length is 0%s", "");
	return vm->stack[--vm->stack_length];
}

static union kn_bytecode next(struct kn_vm *vm, size_t *ip) {
	kn_assert(vm->bytecode_length < *ip);
	return vm->bytecode[*ip++];
}

kn_value kn_vm_run(struct kn_vm *vm, size_t ip) {
	kn_value args[KN_OPCODE_MAX_ARITY];
	unsigned int offset;

	while (1) {
		kn_assert(ip <= vm->bytecode_length);
		enum kn_opcode oc = next(vm, &ip).opcode;

		for (unsigned i = 0; i < kn_opcode_arity(oc); ++i)
			args[i] = pop(vm);

		if (kn_opcode_takes_offset(oc))
			offset = next(vm, &ip).offset;

		switch(oc) {
		case KN_OPCODE_PUSH_CONSTANT:
			kn_assert(offset <= vm->constants_length);
			push(vm, vm->constants[offset]);
			break;

		case KN_OPCODE_POP_AND_RET:
			return args[0];

		case KN_OPCODE_JEQ:
		case KN_OPCODE_JNE:
			if (kn_value_to_boolean(args[0]) == (oc == KN_OPCODE_JEQ))
			case KN_OPCODE_JMP:
				ip = offset;
			break;

		case KN_OPCODE_PROMPT: kn_die("todo KN_OPCODE_PROMPT");// = KN_NEW_OPCODE(0, 0, 0),

		case KN_OPCODE_RANDOM:
			push(vm, kn_value_new_integer((kn_integer) rand()));
			break;

		case KN_OPCODE_CALL: kn_die("todo KN_OPCODE_CALL");//   = KN_NEW_OPCODE(1, 0,  0),
		case KN_OPCODE_QUIT: kn_die("todo KN_OPCODE_QUIT");//   = KN_NEW_OPCODE(1, 0,  1),
		case KN_OPCODE_OUTPUT: kn_die("todo KN_OPCODE_OUTPUT");// = KN_NEW_OPCODE(1, 0,  2),
		case KN_OPCODE_DUMP: kn_die("todo KN_OPCODE_DUMP");//   = KN_NEW_OPCODE(1, 0,  3),
		case KN_OPCODE_LENGTH: kn_die("todo KN_OPCODE_LENGTH");// = KN_NEW_OPCODE(1, 0,  4),
		case KN_OPCODE_NOT: kn_die("todo KN_OPCODE_NOT");//    = KN_NEW_OPCODE(1, 0,  5),
		case KN_OPCODE_NEGATE: kn_die("todo KN_OPCODE_NEGATE");// = KN_NEW_OPCODE(1, 0,  6),
		case KN_OPCODE_ASCII: kn_die("todo KN_OPCODE_ASCII");//  = KN_NEW_OPCODE(1, 0,  7),
		case KN_OPCODE_BOX: kn_die("todo KN_OPCODE_BOX");//    = KN_NEW_OPCODE(1, 0,  8),
		case KN_OPCODE_HEAD: kn_die("todo KN_OPCODE_HEAD");//   = KN_NEW_OPCODE(1, 0,  9),
		case KN_OPCODE_TAIL: kn_die("todo KN_OPCODE_TAIL");//   = KN_NEW_OPCODE(1, 0, 10),

		case KN_OPCODE_ADD: kn_die("todo KN_OPCODE_ADD");// = KN_NEW_OPCODE(2, 0, 0),
		case KN_OPCODE_SUB: kn_die("todo KN_OPCODE_SUB");// = KN_NEW_OPCODE(2, 0, 1),
		case KN_OPCODE_MUL: kn_die("todo KN_OPCODE_MUL");// = KN_NEW_OPCODE(2, 0, 2),
		case KN_OPCODE_DIV: kn_die("todo KN_OPCODE_DIV");// = KN_NEW_OPCODE(2, 0, 3),
		case KN_OPCODE_MOD: kn_die("todo KN_OPCODE_MOD");// = KN_NEW_OPCODE(2, 0, 4),
		case KN_OPCODE_POW: kn_die("todo KN_OPCODE_POW");// = KN_NEW_OPCODE(2, 0, 5),
		case KN_OPCODE_LTH: kn_die("todo KN_OPCODE_LTH");// = KN_NEW_OPCODE(2, 0, 6),
		case KN_OPCODE_GTH: kn_die("todo KN_OPCODE_GTH");// = KN_NEW_OPCODE(2, 0, 7),
		case KN_OPCODE_EQL: kn_die("todo KN_OPCODE_EQL");// = KN_NEW_OPCODE(2, 0, 8),

		case KN_OPCODE_GET: kn_die("todo KN_OPCODE_GET");// = KN_NEW_OPCODE(3, 0, 0),
		case KN_OPCODE_SET: kn_die("todo KN_OPCODE_SET");// = KN_NEW_OPCODE(4, 0, 0),
		default:
			kn_bug("unknown opcode");
		}
	}
}
