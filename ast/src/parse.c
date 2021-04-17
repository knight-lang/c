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

// the stream used by all the parsing functions.
const char *kn_parse_stream;

// Check to see if the character is considered whitespace to Knight.
static int iswhitespace(char c) {
	return isspace(c) || c == ':'
		|| c == '(' || c == ')'
		|| c == '[' || c == ']'
		|| c == '{' || c == '}'
# ifdef KN_EXT_IGNORE_COMMA
		|| c == ','
# endif /* KN_EXT_IGNORE_COMMA */
	;
}

// Checks to see if the character is part of a word function body.
static int iswordfunc(char c) {
	return isupper(c) || c == '_';
}

void kn_parse_strip() {
	char c;

	assert(iswhitespace(c = kn_parse_peek()) || c == '#');

	while (1) {
		c = kn_parse_peek();

		if (KN_UNLIKELY(c == '#')) {
			while ((c = kn_parse_advance_peek()) != '\n' && c != '\0') {
				/* do nothing */
			}
		}

		if (!iswhitespace(c))
			break;

		while (iswhitespace(kn_parse_advance_peek())) {
			/* do nothing */
		}
	}
}

kn_number kn_parse_number() {
	char c = kn_parse_peek();

	assert(isdigit(c));

	kn_number number = (c - '0');

	while (isdigit(c = kn_parse_advance_peek()))
		number = number * 10 + (c - '0');

	return number;
}

struct kn_string *kn_parse_string() {
	char quote = kn_parse_peek_advance();
	char c;
	const char *start = kn_parse_stream;

	assert(quote == '\'' || quote == '\"');

	while (quote != (c = kn_parse_peek_advance())) {

#ifndef KN_RECKLESS
		if (c == '\0')
			die("unterminated quote encountered: '%s'", start);
#endif /* !KN_RECKLESS */

	}

	return kn_string_new_borrowed(start, kn_parse_stream - start - 1);
}

struct kn_variable *kn_parse_variable() {
	const char *start = kn_parse_stream;
	assert(islower(kn_parse_peek()) || kn_parse_peek() == '_');

	char c;

	do {
		c = kn_parse_advance_peek();
	} while (islower(c) || isdigit(c) || c == '_');

	return kn_env_fetch(start, kn_parse_stream - start);
}

struct kn_ast *kn_parse_ast(const struct kn_function *fn) {
	struct kn_ast *ast = kn_ast_alloc(fn->arity);
	ast->func = fn;

	for (size_t i = 0; i < fn->arity; ++i) {
		ast->args[i] = kn_parse_value();

#ifndef KN_RECKLESS
		if (ast->args[i] == KN_UNDEFINED)
			die("unable to parse argument %zu for function '%s'", i, fn->name);
#endif /* !KN_RECKLESS */
	}

	return ast;
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
	kn_parse_advance(); \
	goto parse_function

// Used for functions which are word functions (and can be multiple characters).
#define WORD_FUNC(name, sym) \
	LABEL(function_##name) CASES1(sym) \
	function = &kn_fn_##name; \
	goto parse_kw_function

kn_value kn_parse_value() {
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
		['$']  = &&invalid,
		['%']  = &&function_mod,
		['&']  = &&function_and,
		['\''] = &&string,
		['(']  = &&strip,
		[')']  = &&strip,
		['*']  = &&function_mul,
		['+']  = &&function_add,
# ifdef KN_EXT_IGNORE_COMMA
		[',']  = &&strip,
# else
		[',']  = &&invalid,
# endif /* KN_EXT_IGNORE_COMMA */
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
		['@']  = &&invalid,
		['A']  = &&invalid,
		['B']  = &&function_block,
		['C']  = &&function_call,
		['D']  = &&function_dump,
		['E']  = &&function_eval,
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
		['S']  = &&function_substitute,
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
		['[']  = &&strip,
		['\\'] = &&invalid,
		[']']  = &&strip,
		['^']  = &&function_pow,
		['_']  = &&identifier,
		['`']  = &&function_system,
		['a' ... 'z'] = &&identifier,
		['{']  = &&strip,
		['|']  = &&function_or,
		['}']  = &&strip,

# ifdef KN_EXT_NEGATE
		['~']  = &&function_negate,
# else
		['~']  = &&invalid,
# endif /* KN_EXT_NEGATE */

		[0x7f ... 0xff] = &&invalid
	};
#endif /* KN_COMPUTED_GOTOS */

	char c;
	const struct kn_function *function;

	assert(kn_parse_stream != NULL);

start:
	c = kn_parse_peek();

#ifdef KN_COMPUTED_GOTOS
	goto *labels[(size_t) c];
#else
	switch (c) {
#endif /* KN_COMPUTED_GOTOS */

LABEL(strip)
CASES6('\t', '\n', '\v', '\f', '\r', ' ')
CASES8('(', ')', '[', ']', '{', '}', ':', '#')
#ifdef KN_EXT_IGNORE_COMMA
CASES1(',')
#endif /* KN_EXT_IGNORE_COMMA */
	kn_parse_strip();
	goto start; // go find the next token to return.

LABEL(number)
CASES10('0', '1', '2', '3', '4', '5', '6', '7', '8', '9')
	return kn_value_new_number(kn_parse_number());

LABEL(identifier)
CASES10('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j')
CASES10('k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't')
CASES7('u', 'v', 'w', 'x', 'y', 'z', '_')
	return kn_value_new_variable(kn_parse_variable());

LABEL(string)
CASES2('\'', '\"')
	return kn_value_new_string(kn_parse_string());

LABEL(literal_true)
CASES1('T')
	while(iswordfunc(kn_parse_advance_peek()));
	return KN_TRUE;

LABEL(literal_false)
CASES1('F')
	while(iswordfunc(kn_parse_advance_peek()));
	return KN_FALSE;

LABEL(literal_null)
CASES1('N')
	while(iswordfunc(kn_parse_advance_peek()));
	return KN_NULL;

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
SYMBOL_FUNC(system, '`');

#ifdef KN_EXT_NEGATE
SYMBOL_FUNC(negate, '~');
#endif /* KN_EXT_NEGATE */

WORD_FUNC(block, 'B');
WORD_FUNC(call, 'C');
WORD_FUNC(dump, 'D');
WORD_FUNC(eval, 'E');
WORD_FUNC(get, 'G');
WORD_FUNC(if, 'I');
WORD_FUNC(length, 'L');
WORD_FUNC(output, 'O');
WORD_FUNC(prompt, 'P');
WORD_FUNC(quit, 'Q');
WORD_FUNC(random, 'R');
WORD_FUNC(substitute, 'S');
WORD_FUNC(while, 'W');

#ifdef KN_EXT_VALUE
WORD_FUNC(value, 'V');
#endif /* KN_EXT_VALUE */

parse_kw_function:
	while (iswordfunc(kn_parse_advance_peek())) {
		/* do nothing */
	}
	// fallthrough
parse_function:
	return kn_value_new_ast(kn_parse_ast(function));

LABEL(expected_token)
CASES1('\0')
	return KN_UNDEFINED;

#ifdef KN_CUSTOM
LABEL(extension)
CASES1('X')
	kn_parse_advance(); // delete the `X`
	return kn_parse_extension();
#endif /* KN_CUSTOM */

LABEL(invalid)
#ifndef KN_COMPUTED_GOTOS
default:
#endif /* !KN_COMPUTED_GOTOS */
#ifndef KN_RECKLESS
	die("unknown token start '%c'", c);
#endif /* KN_RECKLESS */

#ifndef KN_COMPUTED_GOTOS
	}
#endif /* !KN_COMPUTED_GOTOS */

	KN_UNREACHABLE();
}

// Actually parses the stream
kn_value kn_parse(const char *stream) {
	kn_parse_stream = stream;
	return kn_parse_value();
}
