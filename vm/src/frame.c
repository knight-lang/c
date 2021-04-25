#include "frame.h"
#include "shared.h"
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

typedef struct {
	unsigned length, capacity;
	bytecode *code;
} frame_code;

#define NBYTECODE (4096*4096)

typedef struct {
	char *input;

	struct {
		unsigned label, local, bytecode;
	} next;

	bytecode bytecode[NBYTECODE];

} parse_context;

static int iswhitespace(char c) {
	return isspace(c)
		|| c == '(' || c == ')'
		|| c == '[' || c == ']'
		|| c == '{' || c == '}'
		|| c == ':';
}

static parse_context ctx;

#define PEEK() (*ctx.input)
#define ADVANCE() (++ctx.input)
#define PEEK_ADVANCE() (*ctx.input++)
#define ADVANCE_PEEK() (*++ctx.input)
#define PUSH_OP(op) (ctx.bytecode[ctx.next.bytecode++].opcode = (op))
#define PUSH_VALUE(val) \
	(PUSH_OP(OP_PUSHL), ctx.bytecode[ctx.next.bytecode++].value = (val))

void parse_frame_inner() {
	char c;

	printf("%p [%c] [%d]\n", ctx.input, PEEK(), PEEK());
	// strip whitespace
	while (iswhitespace(c=PEEK()) || c == '#')
		if (PEEK_ADVANCE() == '#')
			while (PEEK_ADVANCE() != '\n');

	assert(ctx.next.bytecode < NBYTECODE);
	if(c == '\0') DIE("nothing to parse");

	if (isdigit(c)) {
		number num = c - '0';

		while (isdigit(c = PEEK_ADVANCE()))
			num = num * 10 + (c - '0');

		PUSH_VALUE(NUMBER_TO_VALUE(num));
		return;
	}

	if (c == '\'' || c == '\"') {
		ADVANCE();
		char *start = ctx.input;

		while (PEEK_ADVANCE() != c) {
			if(PEEK() == '\0') DIE("unterminated quote");
		}

		unsigned size = ctx.input - start - 1;

		PUSH_VALUE(STRING_TO_VALUE(strndup(start, size)));
		return;
	}

	if (c == 'T' || c == 'F' || c == 'N') {
		while (isupper(PEEK()) || PEEK() == '_')
			ADVANCE();

		PUSH_VALUE(c == 'N' ? V_NULL : c == 'T' ? V_TRUE : V_FALSE);
		return;
	}

	if (islower(c) || c == '_') {
		char *start = ctx.input;

		while (islower(c) || isdigit(c) || c == '_')
			c = ADVANCE_PEEK();

		char *ident = strndup(ctx.input, ctx.input - start);
		// ?????
		(void)ident;
		abort();
	}

	opcode op = c;
	unsigned arity;

	switch(op) {
	case '+':
	case '-':
	case '*':
	case '/': arity = 2; break;
	case 'D': arity = 1; break;
	default: DIE("invalid function '%c'", c);
	}

	if (isupper(c)) {
		do { c = ADVANCE_PEEK(); } while (isupper(c) || c == '_');
	} else {
		ADVANCE();
	}

	for (unsigned i = 0; i < arity; ++i)
		parse_frame_inner();

	PUSH_OP(op);
}

frame *parse_frame(char *input) {
	ctx.next.label = 0;
	ctx.next.local = 0;
	ctx.next.bytecode = 0;
	ctx.input = input;

	parse_frame_inner();

	frame *frame = malloc(sizeof(frame));

	frame->locals = malloc(sizeof(value [frame->num_locals = ctx.next.local]));
	frame->bytecode = malloc(sizeof(bytecode [frame->bytecode_len = ctx.next.bytecode]));
	memcpy(frame->bytecode, ctx.bytecode, 5);

	return frame;
}

void frame_dump(frame *frame) {
	for (unsigned i = 0; i < frame->bytecode_len; ++i) {
		printf("\t% 3d\t", i);

		if (frame->bytecode[i].opcode == OP_PUSHL) {
			printf("$\t");
			value_dump(frame->bytecode[++i].value);
		} else {
			printf("<%1$c> [%1$d]", frame->bytecode[i].opcode);
		}

		printf("\n");
	}
	// unsigned num_locals, bytecode_len;
	// value *locals;
	// bytecode *bytecode;

}

void free_frame(frame *frame) {
	free(frame->locals);
	free(frame->bytecode);
	free(frame);
}
