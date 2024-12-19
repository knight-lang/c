#include "parse.h"
#include "alloc.h"
#include "value.h"
#include "debug.h"
#include "list.h"
#include <ctype.h>

struct parser {
	struct {//todo make all unsigned int
		unsigned int len, cap;
		kn_value *ptr;
	} consts;
	struct {
		size_t len, cap;
		struct variable { const char *ptr; unsigned len; } *ptr;
	} vars;
	struct {
		size_t len, cap;
		union kn_bytecode *ptr;
	} code;

	const char *src;
	size_t len;
};

static char peek(const struct parser *p) {
	kn_assert_ne(p->len, 0);
	return p->src[0];
}

static void advance(struct parser *p) {
	kn_assert_ne(p->len, 0);
	--p->len;
	++p->src;
}

static void unadvance(struct parser *p) {
	++p->len;
	--p->src;
}

static void push(struct parser *p, union kn_bytecode bc) {
	if (p->code.len == p->code.cap)
		p->code.ptr = kn_heap_realloc_ary(p->code.ptr, p->code.cap *= 2, union kn_bytecode);

	p->code.ptr[p->code.len++] = bc;
}

static void push_op(struct parser *p, enum kn_opcode oc) {
	push(p, (union kn_bytecode) { .opcode = oc });
}

static void push_idx(struct parser *p, unsigned int idx) {
	push(p, (union kn_bytecode) { .offset = idx });
}
static unsigned int defer_idx(struct parser *p) {
	unsigned int deferred = p->code.len;
	push_idx(p, 123);
	return deferred;
}

static void do_jump_to_current(struct parser *p, unsigned int idx) {
	p->code.ptr[idx].offset = p->code.len;
}

static void push_const(struct parser *p, kn_value val) {
	unsigned int i;

	for (i = 0; i < p->consts.len; ++i)
		if (kn_value_equal(val, p->consts.ptr[i])) goto found;

	if (p->consts.len == p->consts.cap)
		p->consts.ptr = kn_heap_realloc_ary(p->consts.ptr, p->consts.cap *= 2, kn_value);
	p->consts.ptr[p->consts.len++] = val;

	found:
	push_op(p, KN_OPCODE_PUSH_CONSTANT);
	push_idx(p, i);
}

static void parse_expr(struct parser *p);

struct kn_vm kn_parse(const char *source, size_t length) {
	struct parser p = {
		.consts = { 0, 4, kn_heap_alloc_ary(4, kn_value) },
		.vars = { 0, 4, kn_heap_alloc_ary(4, struct variable) },
		.code = { 0, 16, kn_heap_alloc_ary(16, union kn_bytecode) },
		.src = source,
		.len = length
	};

	parse_expr(&p);
	push_op(&p, KN_OPCODE_STOP);

#ifdef KN_LOG
	for (unsigned i = 0; i < p.code.len; ++i) {
		enum kn_opcode op = p.code.ptr[i].opcode;
		kn_logn("vm[%u] = %s", i, kn_opcode_to_str(op));

		if (kn_opcode_takes_offset(op)) {
			kn_logn(" (offset=%u)", p.code.ptr[++i].offset);
			if (op == KN_OPCODE_PUSH_CONSTANT)
				kn_logn(" : "), kn_value_dump(p.consts.ptr[p.code.ptr[i].offset], stdout);
			if (op == KN_OPCODE_PUSHVAR || op == KN_OPCODE_STOREVAR) {
				kn_logn(" : %.*s", p.vars.ptr[p.code.ptr[i].offset].len, p.vars.ptr[p.code.ptr[i].offset].ptr);
			}

		}
		kn_log("");
	}
#endif

	return (struct kn_vm) {
		.constants = p.consts.ptr,
		.variables = (kn_heap_free(p.vars.ptr), kn_heap_alloc_ary(p.vars.len, kn_value)), // todo
		.bytecode = p.code.ptr
#ifndef KN_NDEBUG
		, .bytecode_length = p.code.len,
		.variables_length = p.vars.len,
		.constants_length = p.consts.len,
#endif
	};
}

static int iswhitespace(char c) {
	return isspace(c) || c == ':' || c == '(' || c == ')';
}

// Checks to see if the character is part of a word function body.
static int iswordfunc(char c) {
	return isupper(c) || c == '_';
}

static void parse_fn(struct parser *p, enum kn_opcode op) {
	for (unsigned i = 0; i < kn_opcode_arity(op); ++i)
		parse_expr(p);
	push_op(p, op);
}


static kn_integer parse_integer(struct parser *p) {
	kn_assert(isdigit(peek(p)));

	kn_integer integer = 0;

	char c;
	while (p->len && isdigit(c = peek(p))) {
		integer = integer * 10 + (kn_integer) (c - '0');
		advance(p);
	}

	return integer;
}

static struct kn_string *parse_string(struct parser *p, char quote) {
	const char *start = p->src;
	kn_assert(quote == '\'' || quote == '\"');

	size_t len = p->len;
	char c;

	while (p->len && quote != (c = peek(p))) {
		if (c == '\0')
			kn_die("nul is not allowed in knight strings");
		advance(p);
	}

	if (!p->len)
		kn_die("unterminated quote encountered: '%s'", start);

	kn_assert(peek(p) == quote);
	advance(p);

	return kn_string_new_borrowed(start, len - p->len - 1);
}

static unsigned int parse_var(struct parser *p) {
	kn_assert(p->len);
	kn_assert(islower(peek(p)) || peek(p) == '_');

	size_t startpos = p->len;
	const char *start = p->src;
	char c;

	while (p->len && (islower(c = peek(p)) || isdigit(c) || c == '_')) {
		advance(p);
	}

	unsigned len = startpos - p->len;
	char *strndup(const char *, unsigned long);
	start = strndup(start, len);
	#ifdef KN_LOG
	unsigned long strlen(const char *);
	#endif
	kn_log("adding in[1]: %*.s (len=%zu)", len, start,strlen(start));

	extern int memcmp(const void *, const void *, unsigned long);
	for (unsigned i = 0; i < p->vars.len; ++i) {
		kn_logn("p->vars.ptr[%d] = %.*s", i, p->vars.ptr[i].len, p->vars.ptr[i].ptr);
		kn_logn(", look = %.*s", len, start);
		kn_logn("; len== %d", p->vars.ptr[i].len == len);
		if (p->vars.ptr[i].len == len) {
			kn_logn("; memcmp== %d", memcmp(p->vars.ptr[i].ptr, start, len));
		}

		if (p->vars.ptr[i].len == len && !memcmp(p->vars.ptr[i].ptr, start, len)) {
			kn_log("\tfound!");
			return i;
		}
		kn_log("");
	}

	if (p->vars.len == p->vars.cap)
		p->vars.ptr = kn_heap_realloc_ary(p->vars.ptr, p->vars.cap *= 2, struct variable);

	kn_log("adding in: %*.s (len=%zu)", len, start, len);
	p->vars.ptr[p->vars.len] = (struct variable) {
		.len = len,
		.ptr = start
	};

	return p->vars.len++;
}
static void strip(struct parser *p) {
	while (p->len)
		if (iswhitespace(peek(p)))
			advance(p);
		else if (peek(p) == '#')
			do advance(p); while (p->len && peek(p) != '\n');
		else break;
}

static void parse_expr(struct parser *p) {
	static enum kn_opcode ops_to_fn[] = {
		['A'] = KN_OPCODE_ASCII,
		['D'] = KN_OPCODE_DUMP,
		['C'] = KN_OPCODE_CALL,
		['G'] = KN_OPCODE_GET,
		['L'] = KN_OPCODE_LENGTH,
		['O'] = KN_OPCODE_OUTPUT,
		['P'] = KN_OPCODE_PROMPT,
		['Q'] = KN_OPCODE_QUIT,
		['R'] = KN_OPCODE_RANDOM,
		['S'] = KN_OPCODE_SET,
		['%'] = KN_OPCODE_MOD,
		['*'] = KN_OPCODE_MUL,
		['+'] = KN_OPCODE_ADD,
		['-'] = KN_OPCODE_SUB,
		['/'] = KN_OPCODE_DIV,
		['<'] = KN_OPCODE_LTH,
		['>'] = KN_OPCODE_GTH,
		['?'] = KN_OPCODE_EQL,
		['^'] = KN_OPCODE_POW,
		['~'] = KN_OPCODE_NEG,
		['!'] = KN_OPCODE_NOT,
		[','] = KN_OPCODE_BOX,
		['['] = KN_OPCODE_HEAD,
		[']'] = KN_OPCODE_TAIL,
	};
	char c;

	strip(p);

	c = peek(p);
	advance(p);
	if (isupper(c)) while (p->len && isupper(peek(p))) advance(p);

	switch (c) {
	case 'B':
		push_op(p, KN_OPCODE_JMP);
		unsigned after_block = defer_idx(p);
		unsigned block_pos = p->code.len;
		parse_expr(p);
		push_op(p, KN_OPCODE_POP_AND_RET);
		do_jump_to_current(p, after_block);
		push_const(p, ((kn_value) block_pos << 3) | KN_VTAG_BLOCKREF);
		break;

	case 'T': push_const(p, KN_TRUE); break;
	case 'F': push_const(p, KN_FALSE); break;
	case 'N': push_const(p, KN_NULL); break;
	case '@': push_const(p, kn_value_new_list(&kn_list_empty)); break;

	case ';':
		parse_expr(p);
		push_op(p, KN_OPCODE_POP);
		parse_expr(p);
		break;

	case '&':
	case '|':
		parse_expr(p);
		push_op(p, KN_OPCODE_DUP);
		push_op(p, c == '&' ? KN_OPCODE_JIFF : KN_OPCODE_JIFT);
		unsigned to_else = defer_idx(p);
		push_op(p, KN_OPCODE_POP);
		parse_expr(p);
		do_jump_to_current(p, to_else);
		break;

	case 'I': {
		parse_expr(p);
		push_op(p, KN_OPCODE_JIFF);
		unsigned to_else = defer_idx(p);

		parse_expr(p);
		push_op(p, KN_OPCODE_JMP);
		unsigned to_end = defer_idx(p);

		do_jump_to_current(p, to_else);
		parse_expr(p);

		do_jump_to_current(p, to_end);
		break;
	}

	case 'W': {
		unsigned int start = p->code.len;
		parse_expr(p);
		push_op(p, KN_OPCODE_JIFF);
		unsigned to_end = defer_idx(p);

		parse_expr(p);
		push_op(p, KN_OPCODE_POP);
		push_op(p, KN_OPCODE_JMP);
		push_idx(p, start);

		do_jump_to_current(p, to_end);
		push_const(p, KN_NULL);
		break;
	}

	case '=': {
		strip(p);
		unsigned var = parse_var(p);
		parse_expr(p);
		push_op(p, KN_OPCODE_STOREVAR);
		push_idx(p, var);
		break;
	}

	case '\"':
	case '\'':
		push_const(p, kn_value_new_string(parse_string(p, c)));
		break;

	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
	case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
	case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '_':
		unadvance(p);
		push_op(p, KN_OPCODE_PUSHVAR);
		push_idx(p, parse_var(p));
		break;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		unadvance(p);
		push_const(p, kn_value_new_integer(parse_integer(p)));
		break;

	default:;
		enum kn_opcode oc = ops_to_fn[(unsigned) c];
		if (oc == 0) kn_die("invalid token start: %c", c);
		parse_fn(p, oc);
		break;
	}
}
