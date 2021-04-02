#ifndef FRAME_H
#define FRAME_H

#include "value.h"

typedef enum {
	OP_HALT,
	OP_DUMP = 'D',
	OP_PUSHL,

	OP_JMP,
	OP_JZ,

	OP_ADD = '+',
	OP_SUB = '-',
	OP_MUL = '*',
	OP_DIV = '/'
} opcode;

typedef union {
	opcode opcode;
	value value;
} bytecode;

typedef struct {
	unsigned num_locals, bytecode_len;
	value *locals;
	bytecode *bytecode;
} frame;

frame *parse_frame(char *);
void free_frame(frame *);
void frame_dmp(frame *);

#endif
