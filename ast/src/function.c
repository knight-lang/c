#include "function.h" /* prototypes */
#include "knight.h"   /* kn_run */
#include "ast.h"      /* kn_ast_run */
#include "env.h"      /* kn_env_fetch, kn_variable, kn_variable_run,
                         kn_variable_assign */
#include "list.h"  
#include "number.h"  
#include "shared.h"   /* die, xmalloc, xrealloc, kn_hash, kn_hash_acc,
                         KN_LIKELY, KN_UNLIKELY */
#include "string.h"   /* kn_string, kn_string_new_owned, kn_string_new_borrowed,
                         kn_string_alloc, kn_string_free, kn_string_empty,
                         kn_string_deref, kn_string_length, kn_string_cache,
                         kn_string_clone_static, kn_string_cache_lookup,
                         kn_string_equal */
#include "value.h"    /* kn_value, kn_number, KN_TRUE, KN_FALSE, KN_NULL,
                         KN_UNDEFINED, new_value_number, new_value_string,
                         new_value_boolean, kn_value_clone, kn_value_free,
                         kn_value_dump, kn_value_is_number, kn_value_is_boolean,
                         kn_value_is_string, kn_value_is_variable,
                         kn_value_as_number, kn_value_as_string,
                         kn_value_as_variable, kn_value_to_boolean,
                         kn_value_to_number, kn_value_to_string, kn_value_run */
#include <string.h>   /* memcpy, memcmp, strndup, strerror */
#include <assert.h>   /* assert */
#include <stdlib.h>   /* rand, srand, free, exit, size_t, NULL */
#include <stdbool.h>  /* bool */
#include <stdio.h>    /* fflush, fputs, putc, puts, feof, ferror, FILE, getline,
                         clearerr, stdout, stdin, popen, fread, pclose */
#include <time.h>     /* time */
#ifndef KN_RECKLESS
#include <sys/errno.h>
#endif /* !KN_RECKLESS */

void kn_function_startup(void) {
	// all we have to do on startup is seed the random number.
	srand(time(NULL));
}

#define LIST kn_value_as_list
#define STRING kn_value_as_string
#define NUMBER kn_value_as_number
#define AST kn_value_as_ast

#define to_list kn_value_to_list
#define to_boolean kn_value_to_boolean
#define to_string kn_value_to_string
#define to_number kn_value_to_number
#define run kn_value_run
#define new_value kn_value_new

static unsigned container_length(kn_value value) {
	assert(kn_value_is_list(value) || kn_value_is_string(value));

	// NOTE: both strings and lists have the the length in the same spot.
	return ((struct kn_list *) KN_UNMASK(value))->length;
}

#define DECLARE_FUNCTION(func, arity, name) \
	KN_DECLARE_FUNCTION(kn_fn_##func, arity, name)

DECLARE_FUNCTION(prompt, 0, "PROMPT") {
	(void) args;

	if (KN_UNLIKELY(feof(stdin)))
		return KN_NULL; // avoid the allocation.

	size_t length = 0;
	size_t capacity = 1024;
	char *line = xmalloc(capacity);

	while (1) {
		if (!fgets(line + length, capacity - length, stdin)) {
			if (ferror(stdin))
				kn_error("unable to read line from stdin: %s", strerror(errno));
			assert(feof(stdin));
			free(line);
			return KN_NULL;
		}

		length += strlen(line + length);
		if (length != capacity - 1)
			break;
		line = xrealloc(line, capacity *= 2);
	}

	assert(length != 0);
	if (line[length - 1] == '\n') {
		length--;
		if (line[length - 1] == '\r')
			--length;
		line[length] = '\0';
	}

	if (KN_UNLIKELY(!length)) {
		free(line);
		return new_value(&kn_string_empty);
	}

	return new_value(kn_string_new_owned(line, length));
}

DECLARE_FUNCTION(random, 0, "RANDOM") {
	(void) args;
	return new_value((kn_number) rand());
}

#ifdef KN_EXT_EVAL
DECLARE_FUNCTION(eval, 1, "EVAL") {
	struct kn_string *string = kn_value_to_string(args[0]);
	kn_value ret = kn_run(kn_string_deref(string));

	kn_string_free(string);

	return ret;
}
#endif /* KN_EXT_EVAL */

// literally just run its argument. Used with `BLOCK`.
DECLARE_FUNCTION(noop, 1, ":") {
	return run(args[0]);
}

DECLARE_FUNCTION(box, 1, ",") {
	return new_value(kn_list_box(run(args[0])));
}

DECLARE_FUNCTION(head, 1, "[") {
	kn_value ran = run(args[0]);

	if (!kn_value_is_string(ran) && !kn_value_is_list(ran))
		kn_error("can only call `[` on strings and lists");

	if (!container_length(ran))
		kn_error("called `[` on an empty list/string");

	if (kn_value_is_list(ran)) {
		kn_value head = kn_value_clone(LIST(ran)->elements[0]);
		kn_list_free(LIST(ran));
		return head;
	}

	struct kn_string *head = kn_string_new_borrowed(kn_string_deref(STRING(ran)), 1);
	kn_string_free(STRING(ran));
	return new_value(head);
}

DECLARE_FUNCTION(tail, 1, "]") {
	kn_value ran = run(args[0]);

	if (!kn_value_is_string(ran) && !kn_value_is_list(ran))
		kn_error("can only call `]` on strings and lists");

	if (!container_length(ran))
		kn_error("called `]` on an empty list/string");

	if (kn_value_is_list(ran)) {
		struct kn_list *tail = kn_list_alloc(LIST(ran)->length - 1);

		for (unsigned i = 0; i < tail->length; i++)
			tail->elements[i] = kn_value_clone(LIST(ran)->elements[i]);

		kn_list_free(LIST(ran));
		return new_value(tail);
	}

	struct kn_string *tail = kn_string_new_borrowed(
		kn_string_deref(STRING(ran)) + 1,
		STRING(ran)->length - 1
	);
	kn_string_free(STRING(ran));
	return new_value(tail);
}

DECLARE_FUNCTION(block, 1, "BLOCK") {
	assert(kn_value_is_ast(args[0]));
	kn_ast_clone(AST(args[0]));
	return args[0];
}

DECLARE_FUNCTION(call, 1, "CALL") {
	kn_value ran = run(args[0]);

	if (!kn_value_is_ast(ran))
		kn_error("can only CALL `BLOCK`s.");

	// optimize for the case where we are running a non-unique ast.
	if (KN_LIKELY(--AST(ran)->refcount))
		return kn_ast_run(AST(ran));

	AST(ran)->refcount++;
	kn_value ret = kn_ast_run(AST(ran));
	kn_ast_free(AST(ran));
	return ret;
}

#ifdef KN_EXT_SYSTEM
DECLARE_FUNCTION(system, 2, "$") {
	struct kn_string *command = to_string(args[0]);

	if (run(args[1]) != KN_NULL)
		die("only `NULL` for a second arg is currently supported");

	const char *str = kn_string_deref(command);
	FILE *stream = popen(str, "r");

	if (stream == NULL)
		kn_error("unable to execute command '%s'.", str);

	kn_string_free(command);

	size_t tmp;
	size_t capacity = 2048;
	size_t length = 0;
	char *result = xmalloc(capacity);

	// try to read the entire stream's stdout to `result`.
	while (0 != (tmp = fread(result + length, 1, capacity - length, stream))) {
		length += tmp;

		if (length == capacity) {
			capacity *= 2;
			result = xrealloc(result, capacity);
		}
	}

	// Abort if `stream` had an error.
	if (ferror(stream))
		kn_error("unable to read command stream");

	result = xrealloc(result, length + 1);
	result[length] = '\0';

	// Abort if we cant close stream.
	if (pclose(stream) == -1)
		kn_error("unable to close command stream");

	return new_value(kn_string_new_owned(result, length));
}
#endif /* KN_EXT_SYSTEM */

DECLARE_FUNCTION(quit, 1, "QUIT") {
	exit((int) to_number(args[0]));
}

DECLARE_FUNCTION(not, 1, "!") {
	return new_value((kn_boolean) !to_boolean(args[0]));
}

DECLARE_FUNCTION(length, 1, "LENGTH") {
	kn_value ran = run(args[0]);

	if (KN_LIKELY(kn_value_is_list(ran) || kn_value_is_string(ran))) {
		unsigned length = container_length(ran);
		kn_value_free(ran);
		return length;
	}

	if (kn_value_is_number(ran)) {
		kn_number num = NUMBER(ran);
		kn_number length = 0;
		do {
			length++;
			num /= 10;
		} while (num);
		return new_value(length);
	}

	struct kn_list *list = to_list(ran);
	kn_value_free(ran);
	kn_number length = (kn_number) list->length;
	kn_list_free(list);
	return new_value(length);
}

DECLARE_FUNCTION(dump, 1, "DUMP") {
	kn_value ret = run(args[0]);

	kn_value_dump(ret, stdout);
	putchar('\n');

	return ret;
}

DECLARE_FUNCTION(output, 1, "OUTPUT") {
	struct kn_string *string = to_string(args[0]);
	char *penult, *str = kn_string_deref(string);

#ifndef KN_RECKLESS
	clearerr(stdout);
#endif /* !KN_RECKLESS */

	if (!string->length) {
		putc('\n', stdout);
	} else if (KN_UNLIKELY('\\' == *(penult = &str[string->length - 1]))) {
		*penult = '\0'; // replace the trailing `\`, so it wont be printed...
		fputs(str, stdout);
		*penult = '\\'; // ...and then restore it.
		fflush(stdout);
	} else {
		puts(str);
	}

	if (ferror(stdout))
		kn_error("unable to write to stdout");

	kn_string_free(string);
	return KN_NULL;
}

DECLARE_FUNCTION(ascii, 1, "ASCII") {
	kn_value value = run(args[0]);

	// If lhs is a string, convert both to a string and concatenate.
	if (kn_value_is_string(value)) {
		if (!STRING(value)->length)
			kn_error("ASCII called with empty string.");
		return new_value((kn_number) *kn_string_deref(STRING(value)));
	}

	if (!kn_value_is_number(value))
		kn_error("can only call ASCII on numbers and strings.");

	// just check for ASCIIness, not actually full-blown knight correctness. 
	if (NUMBER(value) < 0 || 127 < NUMBER(value))
		kn_error("Numeric value %" PRIdkn " is out of range for ascii char.", NUMBER(value));

	char buf[2] = { NUMBER(value), 0 };
	return new_value(kn_string_new_borrowed(buf, 1));
}

#ifdef KN_EXT_VALUE
DECLARE_FUNCTION(value, 1, "VALUE") {
	struct kn_string *string = to_string(args[0]);
	struct kn_variable *variable = kn_env_fetch(kn_string_deref(string), string->length);

	kn_string_free(string);
	return kn_variable_run(variable);
}
#endif /* KN_EXT_VALUE */

DECLARE_FUNCTION(negate, 1, "~") {
	return new_value((kn_number) -to_number(args[0]));
}

DECLARE_FUNCTION(add, 2, "+") {
	kn_value lhs = run(args[0]);

	switch (kn_tag(args[0])) {
	case KN_TAG_LIST:
		return new_value(kn_list_concat(LIST(lhs), to_list(args[1])));

	case KN_TAG_STRING:
		return new_value(kn_string_concat(STRING(lhs), to_string(args[1])));

	case KN_TAG_NUMBER:
		return new_value(NUMBER(lhs) + to_number(args[1]));

	default:
		kn_error("can only add to strings, numbers, and lists");
	}
}

DECLARE_FUNCTION(sub, 2, "-") {
	kn_value lhs = run(args[0]);

	if (!kn_value_is_number(lhs))
		kn_error("can only subtract from numbers");

	return new_value(NUMBER(lhs) - to_number(args[1]));
}

DECLARE_FUNCTION(mul, 2, "*") {
	kn_value lhs = run(args[0]);
	kn_number rhs = to_number(args[1]);

	if (kn_value_is_number(lhs))
		return new_value(NUMBER(lhs) * rhs);

	if (rhs < 0 || rhs != (kn_number) (unsigned) rhs)
		kn_error("negative or too large an amount given");

	if (!kn_value_is_list(lhs) && !kn_value_is_string(lhs))
		kn_error("can only multiply numbers, strings, and lists");

	// If lhs is a string, convert rhs to a number and multiply.
	if (kn_value_is_string(lhs))
		return new_value(kn_string_repeat(STRING(lhs), rhs));

	return new_value(kn_list_repeat(LIST(lhs), rhs));
}

DECLARE_FUNCTION(div, 2, "/") {
	kn_value lhs = run(args[0]);

	if (!kn_value_is_number(lhs))
		kn_error("can only divide numbers");

	kn_number divisor = to_number(args[1]);

	if (!divisor)
		kn_error("attempted to divide by zero");

	return new_value(NUMBER(lhs) / divisor);
}

DECLARE_FUNCTION(mod, 2, "%") {
	kn_value lhs = run(args[0]);

	if (!kn_value_is_number(lhs))
		kn_error("can only modulo numbers");

	kn_number base = to_number(args[1]);

	if (!base)
		kn_error("attempted to modulo by zero");

	return new_value(NUMBER(lhs) % base);
}

DECLARE_FUNCTION(pow, 2, "^") {
	kn_value lhs = run(args[0]);

	if (kn_value_is_list(lhs)) {
		struct kn_string *sep = to_string(args[1]);
		struct kn_string *joined = kn_list_join(LIST(lhs), sep);

		kn_list_free(LIST(lhs));
		kn_string_free(sep);

		return new_value(joined);
	}

	if (!kn_value_is_number(lhs))
		kn_error("can only exponentiate numbers and lists");

	return new_value((kn_number) powl(NUMBER(lhs), to_number(args[1])));
}

DECLARE_FUNCTION(eql, 2, "?") {
	kn_value lhs = run(args[0]);
	kn_value rhs = run(args[1]);

	kn_boolean eql = kn_value_equal(lhs, rhs);

	kn_value_free(lhs);
	kn_value_free(rhs);

	return new_value(eql);
}

DECLARE_FUNCTION(lth, 2, "<") {
	kn_value lhs = run(args[0]);
	kn_value rhs = run(args[1]);

	kn_boolean less = kn_value_compare(lhs, rhs) < 0;

	kn_value_free(lhs);
	kn_value_free(rhs);

	return new_value(less);
}

DECLARE_FUNCTION(gth, 2, ">") {
	kn_value lhs = run(args[0]);
	kn_value rhs = run(args[1]);

	kn_boolean greater = kn_value_compare(lhs, rhs) > 0;

	kn_value_free(lhs);
	kn_value_free(rhs);

	return new_value(greater);
}

DECLARE_FUNCTION(and, 2, "&") {
	kn_value lhs = run(args[0]);

	// return the lhs if its falsey.
	if (!to_boolean(lhs))
		return lhs;

	kn_value_free(lhs);
	return run(args[1]);
}

DECLARE_FUNCTION(or, 2, "|") {
	kn_value lhs = run(args[0]);

	// return the lhs if its truthy.
	if (to_boolean(lhs))
		return lhs;

	kn_value_free(lhs);
	return run(args[1]);
}

DECLARE_FUNCTION(then, 2, ";") {
	// We ensured the first value was an ast in the parser.
	assert(kn_value_is_ast(args[0]));

	kn_value_free(kn_ast_run(AST(args[0])));
	return run(args[1]);
}

DECLARE_FUNCTION(assign, 2, "=") {
	struct kn_variable *variable;

#ifdef KN_EXT_EQL_INTERPOLATE
	// if it's an identifier, special-case it where we don't evaluate it.
	if (KN_LIKELY(kn_value_is_variable(args[0]))) {
#else
		if (!kn_value_is_variable(args[0]))
			kn_error("can only assign to variables");
#endif /* KN_EXT_EQL_INTERPOLATE */

	variable = kn_value_as_variable(args[0]);

#ifdef KN_EXT_EQL_INTERPOLATE
	} else {
		// otherwise, evaluate the expression, convert to a string,
		// and then use that as the variable.
		struct kn_string *string = to_string(args[0]);
		variable = kn_env_fetch(kn_string_deref(string), kn_string_length(string));
		kn_string_free(string);
	}
#endif /* KN_EXT_EQL_INTERPOLATE */


	kn_value ret = run(args[1]);
	kn_variable_assign(variable, kn_value_clone(ret));
	return ret;
}

DECLARE_FUNCTION(while, 2, "WHILE") {
	if (KN_UNLIKELY(!kn_value_is_ast(args[0]) || !kn_value_is_ast(args[1])))
		goto non_ast;

	// vast majority of the time we'll be executing ASTs for both the condition and the body,
	// so we'll special-case that.
	while (to_boolean(kn_ast_run(AST(args[0]))))
		kn_value_free(kn_ast_run(AST(args[1])));
	return KN_NULL;

non_ast:
	while (to_boolean(args[0]))
		kn_value_free(run(args[1]));
	return KN_NULL;
}

DECLARE_FUNCTION(if, 3, "IF") {
	return run(args[1 + !to_boolean(args[0])]);
}

DECLARE_FUNCTION(get, 3, "GET") {
	kn_value container = run(args[0]);
	kn_number start = to_number(args[1]);
	kn_number length = to_number(args[2]);

	if (start < 0 || start != (kn_number) (unsigned) start)
		kn_error("starting index is negative or too large: %"PRIdkn, start);

	if (length < 0 || length != (kn_number) (unsigned) length)
		kn_error("length is negative or too large: %"PRIdkn, length);

	if (!kn_value_is_string(container) && !kn_value_is_list(container))
		kn_error("can only call GET on lists and strings");

	// NOTE: both strings and lists have the the length in the same spot.
	unsigned container_length = ((struct kn_list *) KN_UNMASK(container))->length;

	if (container_length < start + length) {
		kn_error("invalid bounds given to GET: container length = %u, start+length=%"PRIdkn,
			container_length, start + length);
	}

	if (kn_value_is_list(container))
		return new_value(kn_list_get(LIST(container), start, length));

	return new_value(kn_string_get(STRING(container), start, length));
}

DECLARE_FUNCTION(set, 4, "SET") {
	kn_value container = run(args[0]);
	kn_number start = to_number(args[1]);
	kn_number length = to_number(args[2]);

	if (start < 0 || start != (kn_number) (unsigned) start)
		kn_error("starting index is negative or too large: %"PRIdkn, start);

	if (length < 0 || length != (kn_number) (unsigned) length)
		kn_error("length is negative or too large: %"PRIdkn, length);

	if (!kn_value_is_string(container) && !kn_value_is_list(container))
		kn_error("can only call SET on lists and strings");

	// NOTE: both strings and lists have the the length in the same spot.
	unsigned container_length = ((struct kn_list *) KN_UNMASK(container))->length;

	if (container_length < start + length) {
		kn_error("invalid bounds given to SET: container length = %u, start+length=%"PRIdkn,
			container_length, start + length);
	}

	if (kn_value_is_list(container))
		return new_value(kn_list_set(LIST(container), start, length, to_list(args[3])));

	return new_value(kn_string_set(STRING(container), start, length, to_string(args[3])));
}

