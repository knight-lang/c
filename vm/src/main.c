#include "frame.h"
#include <stdlib.h>
#include <src/knight.h>
#include <src/parse.h>
#include <src/env.h>
#include <src/string.h>
#include <stdio.h>

opcode_t code[100];
kn_value consts[100];
struct kn_variable *globals[100];

int main ()  {
	kn_startup();
	frame_t frame;
	frame.nlocals = 4;
	frame.nglobals = 0;
	frame.nconsts = 0;
	frame.globals = globals;
	frame.consts = consts;
	frame.code = code;

#define DECL_CONST(cnst) frame.consts[frame.nconsts++] = (cnst)
#define DECL_GLOBAL(glbl) frame.globals[frame.nglobals++] = (glbl)
#define	GLOBAL(idx) (~(idx))
#define	LOCAL(idx) ((idx)+1)
#define	CONST(idx) ((idx))
#define pos frame.codelen
#define CODE(kind, value) frame.code[pos++].kind = value

	DECL_GLOBAL(kn_env_fetch("i", 1)); // 0
	DECL_GLOBAL(kn_env_fetch("acc", 3)); // 1

	DECL_CONST(kn_value_new_number(0)); // 0
	DECL_CONST(kn_value_new_number(0)); // 1
	DECL_CONST(kn_value_new_number(11)); // 2
	DECL_CONST(kn_value_new_string(kn_string_new_borrowed("acc: ", 5))); // 3
	DECL_CONST(kn_value_new_number(1)); // 4
	DECL_CONST(kn_value_new_string(kn_string_new_borrowed("done: ", 6))); // 5

	pos = 0;
	// (_0) = i 0
	CODE(bytecode, OP_CLOAD);
	CODE(index, CONST(0));
	CODE(index, GLOBAL(0));
goto run;
	// (_1) = acc 0
	CODE(bytecode, OP_CLOAD);
	CODE(index, CONST(1));
	CODE(index, GLOBAL(1));

	int loop = pos;

	// (_2) 11
	CODE(bytecode, OP_CLOAD);
	CODE(index, CONST(2));
	CODE(index, LOCAL(0));

	// (_2) < acc _2
	CODE(bytecode, OP_LTH);
	CODE(index, GLOBAL(1));
	CODE(index, LOCAL(0));
	CODE(index, LOCAL(0));

	// if !_2, goto end 
	CODE(bytecode, OP_JUMP_IF_FALSE);
	CODE(index, LOCAL(0));
	CODE(index, 0); // will be updated later
	int *dst = &frame.code[pos-1].index;

	// = acc + acc i
	CODE(bytecode, OP_ADD);
	CODE(index, GLOBAL(1));
	CODE(index, GLOBAL(0));
	CODE(index, GLOBAL(1));

	// (_2) 'acc: '
	CODE(bytecode, OP_CLOAD);
	CODE(index, CONST(3));
	CODE(index, LOCAL(0));

	// (_2) + _2 acc
	CODE(bytecode, OP_ADD);
	CODE(index, LOCAL(0));
	CODE(index, GLOBAL(1));
	CODE(index, LOCAL(0));

	// OUTPUT _2
	CODE(bytecode, OP_OUTPUT);
	CODE(index, LOCAL(0));
	CODE(index, LOCAL(0));

	// (_2) 1
	CODE(bytecode, OP_CLOAD);
	CODE(index, CONST(4));
	CODE(index, LOCAL(0));

	// = i + i _2
	CODE(bytecode, OP_ADD);
	CODE(index, GLOBAL(0));
	CODE(index, LOCAL(0));
	CODE(index, GLOBAL(0));

	// goto loop
	CODE(bytecode, OP_JUMP);
	CODE(index, loop);
	*dst = pos;

	// (_2) 'done: '
	CODE(bytecode, OP_CLOAD);
	CODE(index, CONST(5));
	CODE(index, LOCAL(0));

	// (_2) + _2 acc
	CODE(bytecode, OP_ADD);
	CODE(index, LOCAL(0));
	CODE(index, GLOBAL(1));
	CODE(index, LOCAL(0));

	// OUTPUT _2
	CODE(bytecode, OP_OUTPUT);
	CODE(index, LOCAL(0));
	CODE(index, LOCAL(0));

run:
	CODE(bytecode, OP_RETURN);
	CODE(index, LOCAL(0));
	kn_value_dump(run_frame(&frame));

	parse_and_run("\n\
; = i 0                         \n\
; = acc 0                       \n\
; WHILE < i 11                  \n\
	; = acc + acc i             \n\
	; OUTPUT + 'acc: ' acc      \n\
	: = i + i 1                 \n\
: OUTPUT + 'done: ' acc         \n");

	kn_shutdown();
}
