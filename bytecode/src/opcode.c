#include "opcode.h"

const char *kn_opcode_to_str(enum kn_opcode oc) {
	switch(oc) {
	case KN_OPCODE_PUSH_CONSTANT: return "PUSH_CONSTANT";
	case KN_OPCODE_POP_AND_RET: return "POP_AND_RET";
	case KN_OPCODE_POP: return "POP";
	case KN_OPCODE_STOP: return "STOP";
	case KN_OPCODE_DUP: return "DUP";
	case KN_OPCODE_PUSHVAR: return "PUSHVAR";
	case KN_OPCODE_STOREVAR: return "STOREVAR";

	case KN_OPCODE_JMP: return "JMP";
	case KN_OPCODE_JIFT: return "JIFT";
	case KN_OPCODE_JIFF: return "JIFF";

	case KN_OPCODE_PROMPT: return "PROMPT";
	case KN_OPCODE_RANDOM: return "RANDOM";

	case KN_OPCODE_CALL: return "CALL";
	case KN_OPCODE_QUIT: return "QUIT";
	case KN_OPCODE_OUTPUT: return "OUTPUT";
	case KN_OPCODE_DUMP: return "DUMP";
	case KN_OPCODE_LENGTH: return "LENGTH";
	case KN_OPCODE_NOT: return "NOT";
	case KN_OPCODE_NEGATE: return "NEGATE";
	case KN_OPCODE_ASCII: return "ASCII";
	case KN_OPCODE_BOX: return "BOX";
	case KN_OPCODE_HEAD: return "HEAD";
	case KN_OPCODE_TAIL: return "TAIL";
	case KN_OPCODE_NEG: return "NEG";

	case KN_OPCODE_ADD: return "ADD";
	case KN_OPCODE_SUB: return "SUB";
	case KN_OPCODE_MUL: return "MUL";
	case KN_OPCODE_DIV: return "DIV";
	case KN_OPCODE_MOD: return "MOD";
	case KN_OPCODE_POW: return "POW";
	case KN_OPCODE_LTH: return "LTH";
	case KN_OPCODE_GTH: return "GTH";
	case KN_OPCODE_EQL: return "EQL";

	case KN_OPCODE_GET: return "GET";
	case KN_OPCODE_SET: return "SET";
	default: return "<unknown>";
	}
}
