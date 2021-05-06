#include "bytecode.h"

const char *op2str(opcode_t op) {
	switch (op) {
	case OP_PROMPT: return "PROMPT";
	case OP_RANDOM: return "RANDOM";
	case OP_HALT: return "HALT";

	case OP_JMP: return "JUMP";
	case OP_RETURN: return "RETURN";
	case OP_JMPFALSE: return "JMPFALSE";
	case OP_GLOAD: return "GLOAD";
	case OP_CLOAD: return "CLOAD";
	case OP_EVAL: return "EVAL";
	case OP_BLOCK: return "BLOCK";
	case OP_CALL: return "CALL";
	case OP_SYSTEM: return "SYSTEM";
	case OP_QUIT: return "QUIT";
	case OP_NOT: return "NOT";
	case OP_LENGTH: return "LENGTH";
	case OP_DUMP: return "DUMP";
	case OP_OUTPUT: return "OUTPUT";

	case OP_ADD: return "ADD";
	case OP_SUB: return "SUB";
	case OP_MUL: return "MUL";
	case OP_DIV: return "DIV";
	case OP_MOD: return "MOD";
	case OP_POW: return "POW";
	case OP_LTH: return "LTH";
	case OP_GTH: return "GTH";
	case OP_EQL: return "EQL";
	case OP_AND: return "AND";
	case OP_OR: return "OR";
	case OP_THEN: return "THEN";
	case OP_ASSIGN: return "ASSIGN";
	case OP_WHILE: return "WHILE";

	case OP_GSTORE: return "GSTORE";
	case OP_IF: return "IF";
	case OP_GET: return "GET";

	case OP_SUBSTITUTE: return "SUBSTITUTE";
	default: return "<unknown>";
	}
}
