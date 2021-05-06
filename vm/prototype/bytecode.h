#pragma once

#define OPCODE_ARGC(opcode) ((opcode) / 0x20)

typedef enum {
	OP_PROMPT,
	OP_RANDOM,
	OP_HALT,
	OP_JMP,
	OP_RETURN,

	OP_JMPFALSE = 0x21,
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
	OP_GSTORE,

	OP_IF = 0x60,
	OP_GET,

	OP_SUBSTITUTE = 0x80
} opcode_t;

typedef int local_or_global_index_t;
typedef unsigned index_t;
typedef unsigned offset_t;

typedef union {
	opcode_t opcode;
	local_or_global_index_t index;
	index_t offset;
} bytecode_t;


const char *op2str(opcode_t);
