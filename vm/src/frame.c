#include "frame.h"
#include <assert.h>
#include <src/shared.h>
#include <src/function.h>
#include <src/env.h>

frame_t *frame_from(kn_value value) {
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

kn_value kn_fn_prompt_function(kn_value *);
kn_value kn_fn_random_function(kn_value *);
kn_value kn_fn_eval_function(kn_value *);
kn_value kn_fn_block_function(kn_value *);
kn_value kn_fn_call_function(kn_value *);
kn_value kn_fn_system_function(kn_value *);
kn_value kn_fn_quit_function(kn_value *);
kn_value kn_fn_not_function(kn_value *);
kn_value kn_fn_length_function(kn_value *);
kn_value kn_fn_dump_function(kn_value *);
kn_value kn_fn_output_function(kn_value *);
kn_value kn_fn_add_function(kn_value *);
kn_value kn_fn_sub_function(kn_value *);
kn_value kn_fn_mul_function(kn_value *);
kn_value kn_fn_div_function(kn_value *);
kn_value kn_fn_mod_function(kn_value *);
kn_value kn_fn_pow_function(kn_value *);
kn_value kn_fn_lth_function(kn_value *);
kn_value kn_fn_gth_function(kn_value *);
kn_value kn_fn_eql_function(kn_value *);
kn_value kn_fn_and_function(kn_value *);
kn_value kn_fn_or_function(kn_value *);
kn_value kn_fn_then_function(kn_value *);
kn_value kn_fn_assign_function(kn_value *);
kn_value kn_fn_while_function(kn_value *);
kn_value kn_fn_if_function(kn_value *);
kn_value kn_fn_get_function(kn_value *);
kn_value kn_fn_substitute_function(kn_value *);

#define OPCODE(idx) (frame->code[idx])
#define NEXT_INDEX() (OPCODE(ip++).index)
#define NEXT_LOCAL() (locals[NEXT_INDEX()])

kn_value
run_frame(const frame_t *frame)
{
	kn_value locals[frame->nlocals];
	kn_value op_result;
	kn_value fn_args[KN_MAX_ARGC];
	bytecode_t bytecode;

	unsigned ip, sp, idx;

	for (ip = sp = 0 ;; locals[sp++] = op_result) {
	top:
		assert(ip < frame->codelen);

		switch (bytecode = OPCODE(ip++).bytecode) {
		case OP_RETURN:
			idx = NEXT_INDEX();

			for (unsigned i = 0; i < sp; ++i)
				if (i != idx) kn_value_free(locals[i]);

			return locals[idx];

		case OP_GLOAD_FAST:
			op_result = kn_variable_run(frame->globals[NEXT_INDEX()]);
			continue;

		case OP_GSTORE_FAST: {
			struct kn_variable *variable = frame->globals[NEXT_INDEX()];
			op_result = locals[OPCODE(ip++).index];

			kn_variable_assign(variable, kn_value_clone(op_result));
			continue;
		}

		case OP_CLOAD:
			op_result = frame->consts[NEXT_INDEX()];
			continue;

		case OP_JUMP:
			ip = OPCODE(ip).index;
			goto top;

		default:
			; // fallthru
		}

		// else, it's a normal type

		for (idx = 0; idx < BYTECODE_ARGC(bytecode); ++idx)
			fn_args[idx] = NEXT_LOCAL();

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
			assert(0);
		}
	}
}

/*
typedef struct {
	struct {
		unsigned len;
	} locals;

	struct {
		unsigned len;
		kn_value *values;
	} consts;

	struct {
		unsigned len;
		opcode_t *opcode;
	} code;
} frame_t;
*/

