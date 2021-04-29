#include "frame.h"
#include <stdlib.h>
#include <src/knight.h>
#include <src/parse.h>
#include <src/string.h>

int main ()  {
	kn_startup();
	frame_t frame;
	frame.nlocals = 5;
	frame.nglobals = 0;
	frame.nconsts = 2;
	frame.consts = malloc(sizeof(kn_value[2]));
	kn_value consts[6] = {
		kn_value_new_number(0),
		kn_value_new_number(0),
		kn_value_new_number(11),
		kn_value_new_string(kn_string_new_borrowed("acc: ", 5)),
		kn_value_new_number(1),
		kn_value_new_string(kn_string_new_borrowed("done: ", 6))
	};

	frame.codelen = 100;

#define OP(x) { .bytecode = OP_##x }
#define IDX(x) { .index = x }

	static opcode_t code[100] = {
		OP(CLOAD), idx(0)
	};
	frame.code = &code;

#define CODE(kind, value) frame.code[idx++].kind = value;

	CODE(bytecode, OP_CLOAD);
	CODE(bytecode, OP_CLOAD);
	kn_value parsed = kn_parse("\n\
; = i 0                         \n\
; = acc 0                       \n\
; WHILE < i 11                  \n\
	; = acc + acc i             \n\
	; OUTPUT + 'acc: ' acc      \n\
	: = i + i 1                 \n\
: OUTPUT + 'done: ' acc         \n");

	frame.code[0].bytecode = OP_CLOAD;  // sp=0
	frame.code[1].index = 0;
	frame.code[2].bytecode = OP_CLOAD;  // sp=1
	frame.code[3].index = 1;
	frame.code[4].bytecode = OP_ADD;    // sp=2
	frame.code[5].index = 0;
	frame.code[6].index = 1;
	frame.code[7].bytecode = OP_OUTPUT; // sp=3
	frame.code[8].index = 2;
	frame.code[9].bytecode = OP_RETURN; // sp=4
	frame.code[10].index = 2;

	kn_value_dump(run_frame(&frame));

	kn_shutdown();
// typedef struct {
// 	unsigned nlocals, nglobals, nconsts, codelen;

// 	struct kn_variable **globals;
// 	kn_value *consts;
// 	opcode_t *code;
// } frame_t;

	// frame_t *frame;

	// kn_startup();

	// frame = frame_from(kn_parse("; = a 3 : OUTPUT + 'a*4=' * a 4"));
	// kn_value_free(run_frame(frame));

	// kn_shutdown();
}

/*
	kn_value parsed = kn_parse("\n\
; = i 0                         \n\
; = acc 0                       \n\
; WHILE < i 11                  \n\
	; = acc + acc i             \n\
	; OUTPUT + 'acc: ' acc      \n\
	: = i + i 1                 \n\
: OUTPUT + 'done: ' acc         \n");
*/
