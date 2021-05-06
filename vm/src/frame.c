#define KN_EXT_CUSTOM_TYPES

#include "frame.h"
#include <assert.h>
#include <src/parse.h>
#include <src/ast.h>
#include <src/shared.h>
#include <src/function.h>
#include <src/env.h>

#define OPCODE(idx) (frame->code[idx])
#define NEXT_INDEX() (OPCODE(ip++).index)
#define NEXT_VALUE() ((tmp=NEXT_INDEX()) < 0 ? kn_variable_run(frame->globals[-tmp]) : locals[tmp])

#ifdef NDEBUG
#define LOG(...)
#else
#define LOG(msg, ...) printf("[%s:%d] " msg "\n", __func__, __LINE__, __VA_ARGS__);
#endif

kn_value
run_frame(const frame_t *frame)
{
	kn_value locals[frame->nlocals];

	for (unsigned i = 0; i < frame->nlocals; ++i) locals[i] = KN_NULL;

	kn_value op_result;
	kn_value fn_args[KN_MAX_ARGC];
	opcode_t opcode;

	unsigned ip, sp, idx, tmp;

	for (ip = sp = 0 ;;
		(void) ((tmp=NEXT_INDEX()) < 0
			? (kn_variable_assign(frame->globals[-tmp], op_result),1)
			: (locals[tmp] = op_result),1)) {
	top:
		assert(ip < frame->codelen);

		LOG("opcode=%d", OPCODE(ip).opcode);

		switch (opcode = OPCODE(ip++).opcode) {
		case OP_RETURN:
			idx = NEXT_INDEX();

			for (unsigned i = 0; i < frame->nlocals; ++i)
				if (i != idx) kn_value_free(locals[i]);

			return locals[idx];

		case OP_GLOAD:
			op_result = kn_variable_run(frame->globals[NEXT_INDEX()]);
			continue;

		case OP_GSTORE: {
			struct kn_variable *variable = frame->globals[NEXT_INDEX()];
			op_result = NEXT_VALUE();

			kn_variable_assign(variable, kn_value_clone(op_result));
			continue;
		}

		case OP_CLOAD:
			op_result = frame->consts[NEXT_INDEX()];
			continue;

		case OP_JMPFALSE:
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

		for (idx = 0; idx < OPCODE_ARGC(opcode); ++idx) 
			fn_args[idx] = NEXT_VALUE();

#define KNIGHT_FUNCTION(name) \
	op_result = kn_fn_##name##_function(fn_args); continue

		switch(opcode) {
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
			printf("bad opcode: 0x%x\n", opcode);
			assert(0);
		}
	}
}

typedef struct {
	frame_t *frame;
	unsigned globalcap, constcap, codecap;
} frame_parser_t;

#define CONCAT(a, b) a ## b
#define CONCAT2(a, b) CONCAT(a, b)
#define COUNT(n1,n2,n3,n4,n5,...) n5

#define INSTRUCTION(fp, ...) INSTRUCTION_(fp, COUNT(dummy, __VA_ARGS__,3,2,1,0), __VA_ARGS__)
#define INSTRUCTION_(fp, n, oc, ...) \
	do { \
		if ((fp)->codecap + n >= (fp)->frame->codelen) { \
			(fp)->frame->code = xrealloc( \
				(fp)->frame->code, sizeof(bytecode_t[(fp)->codecap*=2])); \
		} \
		(fp)->frame->code[(fp)->frame->codelen++].opcode = (oc); \
		CONCAT2(SET_INDICES,n)((fp), __VA_ARGS__); \
	} while(0)

#define SET_INDICES1(fp)
#define SET_INDICES2(fp, idx1) \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx1);
#define SET_INDICES3(fp, idx1, idx2) \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx1); \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx2);
#define SET_INDICES4(fp, idx1, idx2, idx3) \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx1); \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx2); \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx3);
#define SET_INDICES5(fp, idx1, idx2, idx3, idx4) \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx1); \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx2); \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx3); \
	(fp)->frame->code[(fp)->frame->codelen++].index = (idx4); 

#define SET_NEXT(fp, cap, count, store, kind) \
	if (fp->cap == fp->frame->count) \
		fp->frame->store = xrealloc(fp->frame->store, sizeof(kind[fp->cap *= 2])); \
	fp->frame->store[fp->frame->count++]

#define GLB2IDX(glbl) (~(glbl))
#define CONST2IDX(cnst) (cnst)


opcode_t
func2op(char name)
{
	switch (name) {
	case 'P': return OP_PROMPT;
	case 'R': return OP_RANDOM;

	case 'E': return OP_EVAL;
	case 'B': return OP_BLOCK;
	case 'C': return OP_CALL;
	case '`': return OP_SYSTEM;
	case 'Q': return OP_QUIT;
	case '!': return OP_NOT;
	case 'L': return OP_LENGTH;
	case 'D': return OP_DUMP;
	case 'O': return OP_OUTPUT;

	case '+': return OP_ADD;
	case '-': return OP_SUB;
	case '*': return OP_MUL;
	case '/': return OP_DIV;
	case '%': return OP_MOD;
	case '^': return OP_POW;
	case '<': return OP_LTH;
	case '>': return OP_GTH;
	case '?': return OP_EQL;
	case '&': return OP_AND;
	case '|': return OP_OR;
	case ';': return OP_THEN;
	case '=': return OP_ASSIGN;
	case 'W': return OP_WHILE;

	case 'I': return OP_IF;
	case 'G': return OP_GET;

	case 'S': return OP_SUBSTITUTE;

	default:
		die("invalid name: %c", name);
	}
}

static inline void
reserve_code(frame_parser_t *fp, unsigned length)
{
	if (fp->codecap + length >= fp->frame->codelen)
		fp->frame->code = 
			xrealloc(fp->frame->code, sizeof(bytecode_t[fp->codecap *= 2]));
}

static inline void
set_next_opcode(frame_parser_t *fp, opcode_t op) {
	reserve_code(fp, 1);
	fp->frame->code[fp->frame->codelen++].opcode = op;
}

static inline void
set_next_index(frame_parser_t *fp, unsigned idx) {
	reserve_code(fp, 1);
	fp->frame->code[fp->frame->codelen++].index = idx;
}

static inline unsigned
declare_variable(frame_parser_t *fp, struct kn_variable *variable)
{
	unsigned index;

	for (index = 0; index < fp->frame->nglobals; ++index)
		if (fp->frame->globals[index] == variable) // ie identical pointer
			return index;

	if (fp->globalcap == fp->frame->nglobals)
		fp->frame->globals = xrealloc(
			fp->frame->globals, 
			sizeof(struct kn_variable *[fp->globalcap *= 2])
		);

	fp->frame->globals[index = fp->frame->nglobals++] = variable;
	return index;
}

static inline void
set_next_variable(frame_parser_t *fp, struct kn_variable *variable)
{
	reserve_code(fp, 1);
	SET_NEXT(fp, globalcap, nglobals, globals, struct kn_variable *) = variable;
	INSTRUCTION(fp, OP_GLOAD, GLB2IDX(fp->frame->nglobals - 1), fp->frame->nlocals++);
}

static inline void
set_next_constant(frame_parser_t *fp, kn_value value)
{
	SET_NEXT(fp, constcap, nconsts, consts, kn_value) = value;
	INSTRUCTION(fp, OP_CLOAD, CONST2IDX(fp->frame->nconsts - 1), fp->frame->nlocals++);
}

void
static process_frame(frame_parser_t *fp, kn_value value)
{
	if (!kn_value_is_ast(value)) {
		if (kn_value_is_variable(value))
			set_next_variable(fp, kn_value_as_variable(value));
		else
			set_next_constant(fp, value);
		return;
	}

	struct kn_ast *ast = kn_value_as_ast(value);

	opcode_t op = func2op(*ast->func->name);

	unsigned arity = ast->func->arity, args[arity+1];

	switch (ast->func->name[0]) {
		case 'E': die("todo: OP_EVAL");
		case 'B': die("todo: OP_BLOCK");
		case 'C': die("todo: OP_CALL");
		case '&': die("todo: OP_AND");
		case '|': die("todo: OP_OR");
		case ';': die("todo: OP_THEN");
		case 'W': die("todo: OP_WHILE");
		case '=': {
			process_frame(fp, ast->args[1]);
			// args[0] = set_next_variable
			unsigned arg_index = fp->frame->nlocals - 1;
		}
			// set_next_opcode(fp, op);
			// SET_NEXT_OPCODE(fp, OP_ASSIGN);
			// SET_NEXT_INDEX()
			// fp->frame->code[fp->frame->codelen++].opcode = op;
			// args[i] = fp->frame->nlocals-1;

		case 'I': die("todo: OP_IF");
		default:
			;
			// fallthrough
	}

	for (unsigned i = 0; i < arity; ++i) {
		process_frame(fp, ast->args[i]);
		args[i] = fp->frame->nlocals-1;
	}

	reserve_code(fp, arity);

	set_next_opcode(fp, op);

	for (unsigned i = 0; i < arity; ++i)
		fp->frame->code[fp->frame->codelen++].index = args[i];
	fp->frame->code[fp->frame->codelen++].index = fp->frame->nlocals++;
}

frame_t *
frame_from(kn_value value)
{
	frame_parser_t fp;
	fp.globalcap = 10;
	fp.constcap = 10;
	fp.codecap = 100;

	fp.frame = xmalloc(sizeof(frame_t));
	frame_t *frame = fp.frame;

	frame->refcount = 1;
	frame->nlocals = frame->nglobals = frame->nconsts = frame->codelen = 0;
	frame->globals = xmalloc(sizeof(struct kn_variable *[fp.globalcap]));
	frame->consts = xmalloc(sizeof(kn_value[fp.constcap]));
	frame->code = xmalloc(sizeof(bytecode_t[fp.codecap]));

	process_frame(&fp, value);
	INSTRUCTION(&fp, OP_RETURN, frame->nlocals-1);

	frame->globals = xrealloc(frame->globals, sizeof(struct kn_variable *[frame->nglobals]));
	frame->consts = xrealloc(frame->consts, sizeof(kn_value[frame->nconsts]));
	frame->code = xrealloc(frame->code, sizeof(bytecode_t[frame->codelen]));
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
	dump_frame(frame);

	kn_value ret = run_frame(frame);
	free_frame(frame);

	return ret;
}

void
dump_frame(const frame_t *frame)
{
	printf("frame {\nnlocals=%u\nnglobals=%u\n", frame->nlocals, frame->nglobals);
	for (unsigned i = 0; i < frame->nglobals; ++i) {
		printf("%4u: ", i);
		kn_value_dump(kn_value_new_variable(frame->globals[i]));
		putchar('\n');
	}
	printf("nconsts=%u\n", frame->nconsts);
	for (unsigned i = 0; i < frame->nconsts; ++i) {
		printf("%4u: ", i);
		kn_value_dump(frame->consts[i]);
		putchar('\n');
	}
	printf("codelen=%u\n", frame->codelen);

	for (unsigned i = 0; i < frame->codelen;) {
		opcode_t op = frame->code[i++].opcode;
		printf("[%1$4d:%1$-4x] (%3$02x) %2$-12s", i-1, op2str(op), op);

		for (unsigned j = 0; j <= OPCODE_ARGC(op); ++j) {
			int index = frame->code[i++].index;
			if (j) printf(",  ");

			if (index < 0) printf("var(%.6s)", frame->globals[~index]->name);
			else if (op == OP_CLOAD && !j) kn_value_dump(frame->consts[index]);
			else if (op == OP_JUMP || (op == OP_JMPFALSE && index)) printf("line(%d)", index);
			else printf("local(%d)", index);
		}

		putchar('\n');
		continue;
	}
	putchar('\n');
	fflush(stdout);
	putchar('\n');
}

