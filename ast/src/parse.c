#include <assert.h> /* assert */
#include <stddef.h> /* size_t */
#include <ctype.h>  /* isspace, isdigit, islower, isupper */
#include <string.h> /* strndup, memcpy */

#include "parse.h"    /* prototypes, kn_value, kn_number, kn_value_new_number,
                         kn_value_new_variable, kn_value_new_string,
                         kn_value_new_ast, KN_UNDEFINED, KN_TRUE, KN_FALSE,
                         KN_NULL, kn_function, <all the function definitions> */
#include "string.h"   /* kn_string_new_borrowed */
#include "ast.h"      /* kn_ast, kn_ast_alloc */
#include "shared.h"   /* die, KN_UNREACHABLE */
#include "env.h"      /* kn_variable, kn_env_fetch */
#include "list.h"

static bool is_eof(const struct kn_stream *stream) {
	return stream->length <= stream->position;
}


// Check to see if the character is considered whitespace to Knight.
static int iswhitespace(char c) {
	return isspace(c) || c == ':' || c == '(' || c == ')';
}

// Checks to see if the character is part of a word function body.
static int iswordfunc(char c) {
	return isupper(c) || c == '_';
}

void kn_parse_strip(struct kn_stream *stream) {
	assert(!is_eof(stream));
	assert(iswhitespace(kn_parse_peek(stream)) || kn_parse_peek(stream) == '#');

	while (!is_eof(stream)) {
		char c = kn_parse_peek_advance(stream);

		if (KN_UNLIKELY(c == '#')) {
			while (!is_eof(stream) && (c = kn_parse_peek_advance(stream)) != '\n'){
				// do nothing
			}
		}

		if (!iswhitespace(c)) {
			stream->position--;
			break;
		}

		while (!is_eof(stream) && iswhitespace(kn_parse_peek_advance(stream))) {
			// do nothing
		}
	}
}

kn_number kn_parse_number(struct kn_stream *stream) {
	assert(!is_eof(stream));

	char c = kn_parse_peek_advance(stream);
	assert(isdigit(c));

	kn_number number = (kn_number) (c - '0');

	while (!is_eof(stream) && isdigit(c = kn_parse_peek_advance(stream)))
		number = number*10 + (kn_number) (c - '0');

	return number;
}

struct kn_string *kn_parse_string(struct kn_stream *stream) {
	assert(!is_eof(stream));

	char c, quote = kn_parse_peek_advance(stream);
	size_t start = stream->position;

	assert(quote == '\'' || quote == '\"');

	while (!is_eof(stream) && quote != (c = kn_parse_peek_advance(stream))) {
		if (c == '\0')
			kn_error("nul is not allowed in knight strings");
	}

	if (is_eof(stream))
		kn_error("unterminated quote encountered: '%s'", stream->source + start);

	return kn_string_new_borrowed(
		stream->source + start,
		stream->position - start - 1
	);
}

struct kn_variable *kn_parse_variable(struct kn_stream *stream) {
	assert(!is_eof(stream));
	assert(islower(kn_parse_peek(stream)) || kn_parse_peek(stream) == '_');

	size_t start = stream->position;
	char c;

	while (
		!is_eof(stream)
		&& (islower(c = kn_parse_peek(stream)) || isdigit(c) || c == '_')
	) {
		kn_parse_advance(stream);
	}

	return kn_env_fetch(
		stream->env,
		stream->source + start,
		stream->position - start
	);
}

kn_value kn_parse_ast(struct kn_stream *stream, const struct kn_function *fn) {
	struct kn_ast *ast = kn_ast_alloc(fn->arity);
	ast->function = fn;

	for (size_t i = 0; i < fn->arity; ++i) {
		ast->args[i] = kn_parse_value(stream);

		if (ast->args[i] == KN_UNDEFINED)
			kn_error("unable to parse arg %zu for function '%s'", i, fn->name);
	}

	// Efficiency improvements.
	if (KN_UNLIKELY(fn == &kn_fn_block && !kn_value_is_ast(ast->args[0]))) {
		struct kn_ast *block_arg = kn_ast_alloc(1);
		block_arg->function = &kn_fn_noop;
		block_arg->args[0] = ast->args[0];
		ast->args[0] = kn_value_new(block_arg);
	}

	if (KN_UNLIKELY(fn == &kn_fn_then && !kn_value_is_ast(ast->args[0]))) {
		// Since evaluating anything other than an ast is meaningless (evaluating
		// undefined variables is UB so we choose to just ignore it), if the first
		// value is not an ast, we just return the second function's value.
		kn_value rhs = ast->args[1];
		free(ast);
		return rhs;
	}

	return kn_value_new(ast);
}

static void strip_keyword(struct kn_stream *stream) {
	while (!is_eof(stream) && iswordfunc(kn_parse_peek(stream)))
		kn_parse_advance(stream);
}

// Macros used either for computed gotos or switch statements (the switch
// statement is only used when `KN_COMPUTED_GOTOS` is not defined.)
#ifdef KN_COMPUTED_GOTOS
# define LABEL(x) x:
# define CASES10(a, ...)
# define CASES9(a, ...)
# define CASES8(a, ...)
# define CASES7(a, ...)
# define CASES6(a, ...)
# define CASES5(a, ...)
# define CASES4(a, ...)
# define CASES3(a, ...)
# define CASES2(a, ...)
# define CASES1(a)
#else
# define LABEL(x)
# define CASES10(a, ...)case a: CASES9(__VA_ARGS__)
# define CASES9(a, ...) case a: CASES8(__VA_ARGS__)
# define CASES8(a, ...) case a: CASES7(__VA_ARGS__)
# define CASES7(a, ...) case a: CASES6(__VA_ARGS__)
# define CASES6(a, ...) case a: CASES5(__VA_ARGS__)
# define CASES5(a, ...) case a: CASES4(__VA_ARGS__)
# define CASES4(a, ...) case a: CASES3(__VA_ARGS__)
# define CASES3(a, ...) case a: CASES2(__VA_ARGS__)
# define CASES2(a, ...) case a: CASES1(__VA_ARGS__)
# define CASES1(a) case a:
#endif /* KN_COMPUTED_GOTOS */

// Used for functions which are only a single character, eg `+`.
#define SYMBOL_FUNC(name, sym) \
	LABEL(function_##name) CASES1(sym) \
	function = &kn_fn_##name; \
	kn_parse_advance(stream); \
	goto parse_function

// Used for functions which are word functions (and can be multiple characters).
#define WORD_FUNC(name, sym) \
	LABEL(function_##name) CASES1(sym) \
	function = &kn_fn_##name; \
	goto parse_kw_function

kn_value kn_parse_value(struct kn_stream *stream) {
// the global lookup table, which is used for the slightly-more-efficient, but
// non-standard computed gotos version of the parser.
#ifdef KN_COMPUTED_GOTOS
	static const void *labels[256] = {
		['\0'] = &&expected_token,
		[0x01 ... 0x08] = &&invalid,
		['\t' ... '\r'] = &&strip,
		[0x0e ... 0x1f] = &&invalid,
		[' ']  = &&strip,
		['!']  = &&function_not,
		['"']  = &&string,
		['#']  = &&strip,
#ifdef KN_EXT_SYSTEM
		['$']  = &&function_system,
#else
		['$']  = &&invalid,
#endif /* KN_EXT_SYSTEM */
		['%']  = &&function_mod,
		['&']  = &&function_and,
		['\''] = &&string,
		['(']  = &&strip,
		[')']  = &&strip,
		['*']  = &&function_mul,
		['+']  = &&function_add,
		[',']  = &&function_box,
		['-']  = &&function_sub,
		['.']  = &&invalid,
		['/']  = &&function_div,
		['0' ... '9']  = &&number,
		[':']  = &&strip,
		[';']  = &&function_then,
		['<']  = &&function_lth,
		['=']  = &&function_assign,
		['>']  = &&function_gth,
		['?']  = &&function_eql,
		['@']  = &&literal_empty_list,
		['A']  = &&function_ascii,
		['B']  = &&function_block,
		['C']  = &&function_call,
		['D']  = &&function_dump,
#ifdef KN_EXT_EVAL
		['E']  = &&function_eval,
#else
		['E']  = &&invalid,
#endif /* KN_EXT_EVAL */
		['F']  = &&literal_false,
		['G']  = &&function_get,
		['H']  = &&invalid,
		['I']  = &&function_if,
		['J']  = &&invalid,
		['K']  = &&invalid,
		['L']  = &&function_length,
		['M']  = &&invalid,
		['N']  = &&literal_null,
		['O']  = &&function_output,
		['P']  = &&function_prompt,
		['Q']  = &&function_quit,
		['R']  = &&function_random,
		['S']  = &&function_set,
		['T']  = &&literal_true,
		['U']  = &&invalid,

# ifdef KN_EXT_VALUE
		['V']  = &&function_value,
# else
		['V']  = &&invalid,
# endif /* KN_EXT_VALUE */

		['W']  = &&function_while,
		['Y']  = &&invalid,
# ifdef KN_CUSTOM
		['X']  = &&extension,
# else
		['X']  = &&invalid,
# endif /* KN_CUSTOM */
		['Z']  = &&invalid,
		['[']  = &&function_head,
		['\\'] = &&invalid,
		[']']  = &&function_tail,
		['^']  = &&function_pow,
		['_']  = &&identifier,
		['`']  = &&invalid,
		['a' ... 'z'] = &&identifier,
		['{']  = &&invalid,
		['|']  = &&function_or,
		['}']  = &&invalid,
		['~']  = &&function_negate,
		[0x7f ... 0xff] = &&invalid
	};
#endif /* KN_COMPUTED_GOTOS */

	char c;
	const struct kn_function *function;

start:
	if (is_eof(stream))
		return KN_UNDEFINED;

	c = kn_parse_peek(stream);

#ifdef KN_COMPUTED_GOTOS
	goto *labels[(size_t) c];
#else
	switch (c) {
#endif /* KN_COMPUTED_GOTOS */

LABEL(strip)
CASES10('\t', '\n', '\v', '\f', '\r', ' ', '(', ')', ':', '#')
	kn_parse_strip(stream);
	goto start; // go find the next token to return.

LABEL(number)
CASES10('0', '1', '2', '3', '4', '5', '6', '7', '8', '9')
	return kn_value_new_number(kn_parse_number(stream));

LABEL(identifier)
CASES10('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j')
CASES10('k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't')
CASES7('u', 'v', 'w', 'x', 'y', 'z', '_')
	return kn_value_new_variable(kn_parse_variable(stream));

LABEL(string)
CASES2('\'', '\"')
	return kn_value_new_string(kn_parse_string(stream));

LABEL(literal_true)
CASES1('T')
	while(!is_eof(stream) && iswordfunc(kn_parse_peek_advance(stream)));
	return KN_TRUE;

LABEL(literal_false)
CASES1('F')
	while(!is_eof(stream) && iswordfunc(kn_parse_peek_advance(stream)));
	return KN_FALSE;

LABEL(literal_null)
CASES1('N')
	while(!is_eof(stream) && iswordfunc(kn_parse_peek_advance(stream)));
	return KN_NULL;

LABEL(literal_empty_list)
CASES1('@')
	kn_parse_advance(stream);
	return kn_value_new_list(kn_list_clone(&kn_list_empty));

SYMBOL_FUNC(box, ',');
SYMBOL_FUNC(head, '[');
SYMBOL_FUNC(tail, ']');
SYMBOL_FUNC(not, '!');
SYMBOL_FUNC(add, '+');
SYMBOL_FUNC(sub, '-');
SYMBOL_FUNC(mul, '*');
SYMBOL_FUNC(div, '/');
SYMBOL_FUNC(mod, '%');
SYMBOL_FUNC(pow, '^');
SYMBOL_FUNC(eql, '?');
SYMBOL_FUNC(lth, '<');
SYMBOL_FUNC(gth, '>');
SYMBOL_FUNC(and, '&');
SYMBOL_FUNC(or, '|');
SYMBOL_FUNC(then, ';');
SYMBOL_FUNC(assign, '=');
SYMBOL_FUNC(negate, '~');
#ifdef KN_EXT_SYSTEM
SYMBOL_FUNC(system, '$');
#endif /* KN_EXT_SYSTEM */

LABEL(function_prompt)
CASES1('P') {
	static struct kn_ast ast_prompt = {
		.refcount = { 1 }, // Set it to `1` so nothing will ever deallocate it
		.function = &kn_fn_prompt
	};

	strip_keyword(stream);
	++*kn_refcount(&ast_prompt);
	return kn_value_new_ast(&ast_prompt);
}

LABEL(function_random)
CASES1('R') {
	static struct kn_ast ast_random = {
		.refcount = { 1 }, // Set it to `1` so nothing will ever deallocate it
		.function = &kn_fn_random
	};

	strip_keyword(stream);
	++*kn_refcount(&ast_random);
	return kn_value_new_ast(&ast_random);
}

WORD_FUNC(block, 'B');
WORD_FUNC(call, 'C');
WORD_FUNC(dump, 'D');
WORD_FUNC(get, 'G');
WORD_FUNC(if, 'I');
WORD_FUNC(length, 'L');
WORD_FUNC(output, 'O');
WORD_FUNC(ascii, 'A');
WORD_FUNC(quit, 'Q');
WORD_FUNC(set, 'S');
WORD_FUNC(while, 'W');

#ifdef KN_EXT_EVAL
WORD_FUNC(eval, 'E');
#endif /* KN_EXT_EVAL */

#ifdef KN_EXT_VALUE
WORD_FUNC(value, 'V');
#endif /* KN_EXT_VALUE */

parse_kw_function:
	strip_keyword(stream);
	KN_FALLTHROUGH

parse_function:
	return kn_parse_ast(stream, function);

LABEL(expected_token)
CASES1('\0')
	return KN_UNDEFINED;

#ifdef KN_CUSTOM
LABEL(extension)
CASES1('X')
	kn_parse_advance(stream); // delete the `X`
	return kn_parse_extension(stream);
#endif /* KN_CUSTOM */

LABEL(invalid)
#ifndef KN_COMPUTED_GOTOS
default:
	;
#endif /* !KN_COMPUTED_GOTOS */
#ifndef KN_RECKLESS
	kn_error("unknown token start '%c'", c);
#endif /* KN_RECKLESS */

#ifndef KN_COMPUTED_GOTOS
	}
#endif /* !KN_COMPUTED_GOTOS */

	KN_UNREACHABLE();
}

// Actually parses the stream
kn_value kn_parse(struct kn_stream stream) {
	return kn_parse_value(&stream);
}
