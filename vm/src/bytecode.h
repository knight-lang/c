#pragma once

#define OPCODE_ARGC(opcode) ((opcode) / 0x20)

typedef enum {
	OP_PROMPT,
	OP_RANDOM,
	OP_HALT,
	OP_JUMP,
	OP_RETURN,

	OP_JMPFALSE = 0x21,
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

	OP_ADD = 0x40,
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

	OP_GSTORE = 0x60,
	OP_IF,
	OP_GET,

	OP_SUBSTITUTE = 0x80
} opcode_t;

typedef union {
	opcode_t opcode;
	int index;
} bytecode_t;


const char *op2str(opcode_t);
