#pragma once

#define BYTECODE_ARGC(bytecode) ((bytecode) / 0x20)

typedef enum {
	OP_RETURN = 0x00,
	OP_PROMPT,
	OP_RANDOM,

	OP_JUMP = 0x21,
	OP_GLOAD_FAST,
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

	OP_GSTORE_FAST = 0x60,
	OP_IF,
	OP_GET,

	OP_SUBSTITUTE
} bytecode_t;

typedef union {
	bytecode_t bytecode;
	unsigned index;
} opcode_t;
