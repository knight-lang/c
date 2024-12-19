#ifndef KN_OPCODE_H
#define KN_OPCODE_H

#include "debug.h"

#define KN_OPCODE_ARITY_SHIFT 5
#define KN_OPCODE_TAKE_OFFSET_SHIFT 4
#define KN_NEW_OPCODE(arity, offset, counter) \
	((arity) << KN_OPCODE_ARITY_SHIFT | \
	 (offset) << KN_OPCODE_TAKE_OFFSET_SHIFT | \
	 (counter))
#define KN_OPCODE_MAX_ARITY 4

enum kn_opcode {
	// KN_OPCODE_INVALID = 0
	KN_OPCODE_PUSH_CONSTANT = KN_NEW_OPCODE(0, 1, 0),
	KN_OPCODE_POP_AND_RET   = KN_NEW_OPCODE(1, 0, 12),
	KN_OPCODE_POP   = KN_NEW_OPCODE(1, 0, 13),
	KN_OPCODE_DUP   = KN_NEW_OPCODE(1, 0, 14),

	KN_OPCODE_JMP = KN_NEW_OPCODE(0, 1, 2),
	KN_OPCODE_JIFT = KN_NEW_OPCODE(1, 1, 0),
	KN_OPCODE_JIFF = KN_NEW_OPCODE(1, 1, 1),

	KN_OPCODE_PUSHVAR  = KN_NEW_OPCODE(0, 1, 1),
	KN_OPCODE_STOREVAR = KN_NEW_OPCODE(1, 1, 2),

	KN_OPCODE_PROMPT = KN_NEW_OPCODE(0, 0, 1),
	KN_OPCODE_RANDOM = KN_NEW_OPCODE(0, 0, 2),
	KN_OPCODE_STOP   = KN_NEW_OPCODE(0, 0, 3),

	KN_OPCODE_CALL   = KN_NEW_OPCODE(1, 0,  0),
	KN_OPCODE_QUIT   = KN_NEW_OPCODE(1, 0,  1),
	KN_OPCODE_OUTPUT = KN_NEW_OPCODE(1, 0,  2),
	KN_OPCODE_DUMP   = KN_NEW_OPCODE(1, 0,  3),
	KN_OPCODE_LENGTH = KN_NEW_OPCODE(1, 0,  4),
	KN_OPCODE_NOT    = KN_NEW_OPCODE(1, 0,  5),
	KN_OPCODE_NEGATE = KN_NEW_OPCODE(1, 0,  6),
	KN_OPCODE_ASCII  = KN_NEW_OPCODE(1, 0,  7),
	KN_OPCODE_BOX    = KN_NEW_OPCODE(1, 0,  8),
	KN_OPCODE_HEAD   = KN_NEW_OPCODE(1, 0,  9),
	KN_OPCODE_TAIL   = KN_NEW_OPCODE(1, 0, 10),
	KN_OPCODE_NEG    = KN_NEW_OPCODE(1, 0, 11),

	KN_OPCODE_ADD = KN_NEW_OPCODE(2, 0, 0),
	KN_OPCODE_SUB = KN_NEW_OPCODE(2, 0, 1),
	KN_OPCODE_MUL = KN_NEW_OPCODE(2, 0, 2),
	KN_OPCODE_DIV = KN_NEW_OPCODE(2, 0, 3),
	KN_OPCODE_MOD = KN_NEW_OPCODE(2, 0, 4),
	KN_OPCODE_POW = KN_NEW_OPCODE(2, 0, 5),
	KN_OPCODE_LTH = KN_NEW_OPCODE(2, 0, 6),
	KN_OPCODE_GTH = KN_NEW_OPCODE(2, 0, 7),
	KN_OPCODE_EQL = KN_NEW_OPCODE(2, 0, 8),

	KN_OPCODE_GET = KN_NEW_OPCODE(3, 0, 0),
	KN_OPCODE_SET = KN_NEW_OPCODE(4, 0, 0),
};
#undef KN_NEW_OPCODE

const char *kn_opcode_to_str(enum kn_opcode oc);

static inline unsigned char kn_opcode_arity(enum kn_opcode opcode) {
	return (unsigned char) opcode >> KN_OPCODE_ARITY_SHIFT;
}

static inline _Bool kn_opcode_takes_offset(enum kn_opcode opcode) {
	return opcode & (1 << KN_OPCODE_TAKE_OFFSET_SHIFT);
}

typedef unsigned int kn_offset;

union kn_bytecode {
	enum kn_opcode opcode;
	kn_offset offset;
};


#endif
