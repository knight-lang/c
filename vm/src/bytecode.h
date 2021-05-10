#ifndef KN_BYTECODE_H
#define KN_BYTECODE_H

#define OPCODE_ARGC(opcode) ((opcode) / 0x20)

enum kn_opcode {
	OP_PROMPT,
	OP_RANDOM,
	OP_HALT,
	OP_JMP,
	OP_RETURN,

	OP_JMP_FALSE = 0x20,
	OP_MOV,
	OP_GLOAD,
	OP_CLOAD,
	OP_EVAL,
	OP_BLOCK,
	OP_CALL,
	OP_SYSTEM,
	OP_QUIT,
	OP_NOT,
	OP_LENGTH,
	OP_DUMP,
	OP_OUTPUT,

	OP_GSTORE = 0x40,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_POW,
	OP_LTH,
	OP_GTH,
	OP_EQL,
	OP_AND,
	OP_OR,
	OP_THEN,
	OP_ASSIGN,
	OP_WHILE,

	OP_IF = 0x60,
	OP_GET,

	OP_SUBSTITUTE = 0x80
};

union kn_bytecode {
	enum kn_opcode opcode;
	unsigned offset;
};

const char *kn_opcode_to_str(enum kn_opcode opcode);

#endif /* KN_BYTECODE_H */
