#define KN_EXT_CUSTOM_TYPES

#include "frame.h"
#include <assert.h>
#include <src/parse.h>
#include <src/shared.h>
#include <src/function.h>
#include <src/env.h>

// #define EXTERN_KN_FN(name) extern kn_value kn_fn_##name##_function(kn_value *)

// EXTERN_KN_FN(prompt);
// EXTERN_KN_FN(random);
// EXTERN_KN_FN(eval);
// EXTERN_KN_FN(block);
// EXTERN_KN_FN(call);
// EXTERN_KN_FN(system);
// EXTERN_KN_FN(quit);
// EXTERN_KN_FN(not);
// EXTERN_KN_FN(length);
// EXTERN_KN_FN(dump);
// EXTERN_KN_FN(output);
// EXTERN_KN_FN(add);
// EXTERN_KN_FN(sub);
// EXTERN_KN_FN(mul);
// EXTERN_KN_FN(div);
// EXTERN_KN_FN(mod);
// EXTERN_KN_FN(pow);
// EXTERN_KN_FN(lth);
// EXTERN_KN_FN(gth);
// EXTERN_KN_FN(eql);
// EXTERN_KN_FN(and);
// EXTERN_KN_FN(or);
// EXTERN_KN_FN(then);
// EXTERN_KN_FN(assign);
// EXTERN_KN_FN(while);
// EXTERN_KN_FN(if);
// EXTERN_KN_FN(get);
// EXTERN_KN_FN(substitute);


#define OPCODE(idx) (frame->code[idx])
#define NEXT_INDEX() (OPCODE(ip++).index)
#define NEXT_VALUE() ((tmp=NEXT_INDEX()) < 0 ? kn_variable_run(frame->globals[-tmp]) : locals[tmp])

kn_value
run_frame(const frame_t *frame)
{
	kn_value locals[frame->nlocals];

	for (unsigned i = 0; i < frame->nlocals; ++i) locals[i] = KN_NULL;

	kn_value op_result;
	kn_value fn_args[KN_MAX_ARGC];
	bytecode_t bytecode;

	unsigned ip, sp, idx, tmp;

	for (ip = sp = 0 ;;
		(void) ((tmp=NEXT_INDEX()) < 0
			? (kn_variable_assign(frame->globals[-tmp], op_result),1)
			: (locals[tmp] = op_result),1)) {
	top:
		assert(ip < frame->codelen);

		switch (bytecode = OPCODE(ip++).bytecode) {
		case OP_RETURN:
			idx = NEXT_INDEX();

			for (unsigned i = 0; i < frame->nlocals; ++i)
				if (i != idx) kn_value_free(locals[i]);

			return locals[idx];

		case OP_GLOAD_FAST:
			op_result = kn_variable_run(frame->globals[NEXT_INDEX()]);
			continue;

		case OP_GSTORE_FAST: {
			struct kn_variable *variable = frame->globals[NEXT_INDEX()];
			op_result = NEXT_VALUE();

			kn_variable_assign(variable, kn_value_clone(op_result));
			continue;
		}

		case OP_CLOAD:
			op_result = frame->consts[NEXT_INDEX()];
			continue;

		case OP_JUMP_IF_FALSE:
			if (kn_value_to_boolean(NEXT_VALUE())) {
				NEXT_INDEX();
				goto top;
			}

			// otheriwse, fallthrough
		case OP_JUMP:
			ip = OPCODE(ip).index;
			goto top;

		default:
			; // fallthru
		}

		// else, it's a normal type

		for (idx = 0; idx < BYTECODE_ARGC(bytecode); ++idx) 
			fn_args[idx] = NEXT_VALUE();

#define KNIGHT_FUNCTION(name) \
	op_result = kn_fn_##name##_function(fn_args); continue

		switch(bytecode) {
		case OP_PROMPT: KNIGHT_FUNCTION(prompt);
		case OP_RANDOM: KNIGHT_FUNCTION(random);
		case OP_EVAL: assert(0);
		case OP_BLOCK: assert(0);
		case OP_CALL: assert(0);
		case OP_SYSTEM: KNIGHT_FUNCTION(system);
		case OP_QUIT: KNIGHT_FUNCTION(quit);
		case OP_NOT: KNIGHT_FUNCTION(not);
		case OP_LENGTH: KNIGHT_FUNCTION(length);
		case OP_DUMP: KNIGHT_FUNCTION(dump);
		case OP_OUTPUT: KNIGHT_FUNCTION(output);
		case OP_ADD: KNIGHT_FUNCTION(add);
		case OP_SUB: KNIGHT_FUNCTION(sub);
		case OP_MUL: KNIGHT_FUNCTION(mul);
		case OP_DIV: KNIGHT_FUNCTION(div);
		case OP_MOD: KNIGHT_FUNCTION(mod);
		case OP_POW: KNIGHT_FUNCTION(pow);
		case OP_LTH: KNIGHT_FUNCTION(lth);
		case OP_GTH: KNIGHT_FUNCTION(gth);
		case OP_EQL: KNIGHT_FUNCTION(eql);
		case OP_AND: KNIGHT_FUNCTION(and);
		case OP_OR:  KNIGHT_FUNCTION(or);
		case OP_THEN: KNIGHT_FUNCTION(then);
		case OP_ASSIGN: KNIGHT_FUNCTION(assign);
		case OP_WHILE: KNIGHT_FUNCTION(while);
		case OP_IF: KNIGHT_FUNCTION(if);
		case OP_GET: KNIGHT_FUNCTION(get);
		case OP_SUBSTITUTE: KNIGHT_FUNCTION(substitute);
		default:
			printf("bad bytecode: 0x%x\n", bytecode);
			assert(0);
		}
	}
}

frame_t *
frame_from(kn_value value)
{
	frame_t *frame = xmalloc(sizeof(frame_t));
 
	// frame->locals.len = 2;
	// frame->consts.len = 3;
/*
; = a 3
: OUTPUT + 'a*4=' * a 4

; = a 3
; = _0 'a*4='
; = _2 0
; = _1 * a _0
; = 
: OUTPUT + 'a*4=' * a 4


c0 = 3
c1 = 'a*4='
c2 = 4

l0 = 3
l1 = 

*/

	(void) value;

	return frame;
}

void
clone_frame(frame_t *frame)
{
	frame->refcount++;
}

void
free_frame(frame_t *frame)
{
	if (!--frame->refcount) return;

	free(frame->code);
	free(frame->globals); // dont need to free variables

	for (unsigned i = 0 ; i < frame->nconsts; ++i)
		kn_value_free(frame->consts[i]);

	free(frame->consts);
	free(frame);
}

kn_value
parse_and_run(const char *stream)
{
	kn_value parsed = kn_parse(stream);

	if (parsed == KN_UNDEFINED) die("cannot parse stream");

	frame_t *frame = frame_from(parsed);
	kn_value ret = run_frame(frame);
	free_frame(frame);

	return ret;
}
