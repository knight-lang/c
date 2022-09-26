#include "function.h" /* prototypes */
#include "knight.h"   /* kn_play */
#include "ast.h"      /* kn_ast_run */
#include "env.h"      /* kn_env_fetch, kn_variable, kn_variable_run,
                         kn_variable_assign */
#include "list.h"  
#include "integer.h"  
#include "shared.h"   /* die, xmalloc, xrealloc, kn_hash, kn_hash_acc,
                         KN_LIKELY, KN_UNLIKELY */
#include "string.h"   /* kn_string, kn_string_new_owned, kn_string_new_borrowed,
                         kn_string_alloc, kn_string_free, kn_string_empty,
                         kn_string_deref, kn_string_length, kn_string_cache,
                         kn_string_clone_static, kn_string_cache_lookup,
                         kn_string_equal */
#include "value.h"    /* kn_value, kn_integer, KN_TRUE, KN_FALSE, KN_NULL,
                         KN_UNDEFINED, new_value_number, new_value_string,
                         new_value_boolean, kn_value_clone, kn_value_free,
                         kn_value_dump, kn_value_is_integer, kn_value_is_boolean,
                         kn_value_is_string, kn_value_is_variable,
                         kn_value_as_integer, kn_value_as_string,
                         kn_value_as_variable, kn_value_to_boolean,
                         kn_value_to_integer, kn_value_to_string, kn_value_run */
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
#ifdef KN_FUZZING
	srand(0); // we want deterministic inputs
#else
	srand(time(NULL));
#endif  /* KN_FUZZING */
}

#define DECLARE_FUNCTION(func, arity, name) \
	KN_DECLARE_FUNCTION(kn_fn_##func, arity, name)

DECLARE_FUNCTION(prompt, 0, "PROMPT") {
	(void) args;

#ifdef KN_FUZZING
	// Don't read from stdin during fuzzing.
	if (true)
		return kn_value_new(&kn_string_empty);
#endif /* KN_FUZZING */

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
		if (length && line[length - 1] == '\r')
			--length;
		line[length] = '\0';
	}

	if (KN_UNLIKELY(length == 0)) {
		free(line);
		return kn_value_new(&kn_string_empty);
	}

	return kn_value_new(kn_string_new_owned(line, length));
}

DECLARE_FUNCTION(random, 0, "RANDOM") {
	(void) args;

	return kn_value_new((kn_integer) rand());
}

#ifdef KN_EXT_EVAL
DECLARE_FUNCTION(eval, 1, "EVAL") {
	struct kn_string *string = kn_value_to_string(args[0]);
	kn_value ret = kn_play(kn_string_deref(string));

	kn_string_free(string);
	return ret;
}
#endif /* KN_EXT_EVAL */

// literally just run its argument. Used with `BLOCK`.
DECLARE_FUNCTION(noop, 1, ":") {
	return kn_value_run(args[0]);
}

DECLARE_FUNCTION(box, 1, ",") {
	struct kn_list *list = kn_list_alloc(1);

	assert(list->flags & KN_LIST_FL_EMBED);
	list->embed[0] = kn_value_run(args[0]);

	return kn_value_new(list);
}

DECLARE_FUNCTION(head, 1, "[") {
	kn_value ran = kn_value_run(args[0]);

	if (!kn_value_is_string(ran) && !kn_value_is_list(ran))
		kn_error("can only call `[` on strings and lists");

	if (kn_container_length(ran) == 0)
		kn_error("called `[` on an empty list/string");

	if (kn_value_is_list(ran)) {
		struct kn_list *list = kn_value_as_list(ran);
		kn_value head = kn_value_clone(kn_list_get(list, 0));

		kn_list_free(list);
		return head;
	} else {
		struct kn_string *string = kn_value_as_string(ran);
		struct kn_string *head = kn_string_new_borrowed(kn_string_deref(string), 1);

		kn_string_free(string);
		return kn_value_new(head);
	}
}

DECLARE_FUNCTION(tail, 1, "]") {
	kn_value ran = kn_value_run(args[0]);

	if (!kn_value_is_string(ran) && !kn_value_is_list(ran))
		kn_error("can only call `]` on strings and lists");

	if (kn_container_length(ran) == 0)
		kn_error("called `]` on an empty list/string");

	if (kn_value_is_list(ran)) {
		struct kn_list *list = kn_value_as_list(ran);
		return kn_value_new(kn_list_get_sublist(list, 1, kn_length(list) - 1));
	} else {
		struct kn_string *string = kn_value_as_string(ran);
		return kn_value_new(kn_string_get_substring(string, 1, kn_length(string) - 1));
	}
}

DECLARE_FUNCTION(block, 1, "BLOCK") {
	assert(kn_value_is_ast(args[0])); // should have been taken care of during parsing.

	kn_ast_clone(kn_value_as_ast(args[0]));

	return args[0];
}

DECLARE_FUNCTION(call, 1, "CALL") {
	kn_value ran = kn_value_run(args[0]);

	if (!kn_value_is_ast(ran))
		kn_error("can only CALL `BLOCK`s.");

	struct kn_ast *ast = kn_value_as_ast(ran);

	// Optimize for the case where we are running a non-unique ast.
	if (KN_LIKELY(--*kn_refcount(ast) != 0))
		return kn_ast_run(ast);

	++*kn_refcount(ast); // We subtracted one, so now we have to add.
	kn_value ret = kn_ast_run(ast);
	kn_ast_free(ast);
	return ret;
}

#ifdef KN_EXT_SYSTEM
DECLARE_FUNCTION(system, 2, "$") {
	struct kn_string *command = kn_value_to_string(args[0]);

	if (kn_value_run(args[1]) != KN_NULL)
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

	return kn_value_new(kn_string_new_owned(result, length));
}
#endif /* KN_EXT_SYSTEM */

DECLARE_FUNCTION(quit, 1, "QUIT") {
	int status_code = (int) kn_value_to_integer(args[0]);

#ifdef KN_FUZZING
	if (true)
		longjmp(kn_play_start, 1);
#endif /* KN_FUZZING */

	exit(status_code);
}

DECLARE_FUNCTION(not, 1, "!") {
	return kn_value_new((kn_boolean) !kn_value_to_boolean(args[0]));
}

DECLARE_FUNCTION(length, 1, "LENGTH") {
	kn_value ran = kn_value_run(args[0]);

	if (KN_LIKELY(kn_value_is_list(ran) || kn_value_is_string(ran))) {
		size_t length = kn_container_length(ran);

		kn_value_free(ran);
		return kn_value_new((kn_integer) length);
	}

	if (kn_value_is_integer(ran)) {
		kn_integer integer = kn_value_as_integer(ran);
		kn_integer length = 0;

		do {
			length++;
			integer /= 10;
		} while (integer);

		return kn_value_new(length);
	}

	struct kn_list *list = kn_value_to_list(ran);
	kn_integer length = (kn_integer) kn_length(list);

	kn_value_free(ran);
	kn_list_free(list);
	return kn_value_new(length);
}

DECLARE_FUNCTION(dump, 1, "DUMP") {
	kn_value ran = kn_value_run(args[0]);

	kn_value_dump(ran, stdout);

	return ran;
}

DECLARE_FUNCTION(output, 1, "OUTPUT") {
	struct kn_string *string = kn_value_to_string(args[0]);
	char *str = kn_string_deref(string);

#ifndef KN_RECKLESS
	clearerr(stdout);
#endif /* !KN_RECKLESS */

	size_t length = kn_length(string);
	if (length != 0 && str[length - 1] == '\\') {
		fwrite(str, sizeof(char), length - 1, stdout);
	} else {
		fwrite(str, sizeof(char), length, stdout);
		putchar('\n');
	}

	fflush(stdout);
	if (ferror(stdout))
		kn_error("unable to write to stdout");

	kn_string_free(string);
	return KN_NULL;
}

DECLARE_FUNCTION(ascii, 1, "ASCII") {
	kn_value ran = kn_value_run(args[0]);

	if (!kn_value_is_integer(ran) && !kn_value_is_string(ran))
		kn_error("can only call ASCII on integer and strings.");

	// If lhs is a string, convert both to a string and concatenate.
	if (kn_value_is_string(ran)) {
		struct kn_string *string = kn_value_as_string(ran);

		if (kn_length(string) == 0)
			kn_error("ASCII called with empty string.");

		char head = kn_string_deref(string)[0];
		kn_string_free(string);

		return kn_value_new((kn_integer) head);
	}

	kn_integer integer = kn_value_as_integer(ran);

	// just check for ASCIIness, not actually full-blown knight correctness. 
	if (integer <= 0 || 127 < integer)
		kn_error("Integer %" PRIdkn " is out of range for ascii char.", integer);

	char buf[2] = { integer, 0 };
	return kn_value_new(kn_string_new_borrowed(buf, 1));
}

#ifdef KN_EXT_VALUE
DECLARE_FUNCTION(value, 1, "VALUE") {
	struct kn_string *string = kn_value_to_string(args[0]);
	struct kn_variable *variable = kn_env_fetch(kn_string_deref(string), kn_length(string));

	kn_string_free(string);
	return kn_variable_run(variable);
}
#endif /* KN_EXT_VALUE */

DECLARE_FUNCTION(negate, 1, "~") {
	return kn_value_new((kn_integer) -kn_value_to_integer(args[0]));
}

DECLARE_FUNCTION(add, 2, "+") {
	kn_value lhs = kn_value_run(args[0]);

	switch (kn_tag(lhs)) {
	case KN_TAG_LIST:
		return kn_value_new(kn_list_concat(
			kn_value_as_list(lhs),
			kn_value_to_list(args[1])
		));

	case KN_TAG_STRING:
		return kn_value_new(kn_string_concat(
			kn_value_as_string(lhs),
			kn_value_to_string(args[1])
		));

	case KN_TAG_INTEGER:
		return kn_value_new(
			kn_value_as_integer(lhs) + kn_value_to_integer(args[1])
		);

	default:
		kn_error("can only add to strings, integers, and lists");
	}
}

DECLARE_FUNCTION(sub, 2, "-") {
	kn_value lhs = kn_value_run(args[0]);

	if (!kn_value_is_integer(lhs))
		kn_error("can only subtract from integers");

	return kn_value_new(kn_value_as_integer(lhs) - kn_value_to_integer(args[1]));
}

DECLARE_FUNCTION(mul, 2, "*") {
	kn_value lhs = kn_value_run(args[0]);
	kn_integer rhs = kn_value_to_integer(args[1]);

	if (kn_value_is_integer(lhs))
		return kn_value_new(kn_value_as_integer(lhs) * rhs);

	if (rhs < 0 || rhs != (kn_integer) (size_t) rhs)
		kn_error("negative or too large an amount given");

	if (!kn_value_is_list(lhs) && !kn_value_is_string(lhs))
		kn_error("can only multiply integers, strings, and lists");

	// If lhs is a string, convert rhs to an integer and multiply.
	if (kn_value_is_string(lhs))
		return kn_value_new(kn_string_repeat(kn_value_as_string(lhs), rhs));

	return kn_value_new(kn_list_repeat(kn_value_as_list(lhs), rhs));
}

DECLARE_FUNCTION(div, 2, "/") {
	kn_value lhs = kn_value_run(args[0]);

	if (!kn_value_is_integer(lhs))
		kn_error("can only divide integers");

	kn_integer divisor = kn_value_to_integer(args[1]);

	if (divisor == 0)
		kn_error("attempted to divide by zero");

	return kn_value_new(kn_value_as_integer(lhs) / divisor);
}

DECLARE_FUNCTION(mod, 2, "%") {
	kn_value lhs = kn_value_run(args[0]);

	if (!kn_value_is_integer(lhs))
		kn_error("can only modulo integers");

	kn_integer base = kn_value_to_integer(args[1]);

	if (base == 0)
		kn_error("attempted to modulo by zero");

	return kn_value_new(kn_value_as_integer(lhs) % base);
}

DECLARE_FUNCTION(pow, 2, "^") {
	kn_value lhs = kn_value_run(args[0]);

	if (!kn_value_is_list(lhs) && !kn_value_is_integer(lhs))
		kn_error("can only exponentiate integers and lists");

	if (kn_value_is_integer(lhs)) {
		return kn_value_new((kn_integer) powl(
			kn_value_as_integer(lhs),
			kn_value_to_integer(args[1])
		));
	}

	struct kn_list *list = kn_value_as_list(lhs);
	struct kn_string *sep = kn_value_to_string(args[1]);
	struct kn_string *joined = kn_list_join(list, sep);

	kn_list_free(list);
	kn_string_free(sep);
	return kn_value_new(joined);
}

DECLARE_FUNCTION(eql, 2, "?") {
	kn_value lhs = kn_value_run(args[0]);
	kn_value rhs = kn_value_run(args[1]);

	kn_boolean equal = kn_value_equal(lhs, rhs);

	kn_value_free(lhs);
	kn_value_free(rhs);

	return kn_value_new(equal);
}

DECLARE_FUNCTION(lth, 2, "<") {
	kn_value lhs = kn_value_run(args[0]);
	kn_value rhs = kn_value_run(args[1]);

	kn_boolean lesser = kn_value_compare(lhs, rhs) < 0;

	kn_value_free(lhs);
	kn_value_free(rhs);

	return kn_value_new(lesser);
}

DECLARE_FUNCTION(gth, 2, ">") {
	kn_value lhs = kn_value_run(args[0]);
	kn_value rhs = kn_value_run(args[1]);

	kn_boolean greater = kn_value_compare(lhs, rhs) > 0;

	kn_value_free(lhs);
	kn_value_free(rhs);

	return kn_value_new(greater);
}

DECLARE_FUNCTION(and, 2, "&") {
	kn_value lhs = kn_value_run(args[0]);

	// return the lhs if it's falsey.
	if (!kn_value_to_boolean(lhs))
		return lhs;

	kn_value_free(lhs);
	return kn_value_run(args[1]);
}

DECLARE_FUNCTION(or, 2, "|") {
	kn_value lhs = kn_value_run(args[0]);

	// return the lhs if it's truthy.
	if (kn_value_to_boolean(lhs))
		return lhs;

	kn_value_free(lhs);
	return kn_value_run(args[1]);
}

DECLARE_FUNCTION(then, 2, ";") {
	// We ensured the first value was an ast in the parser.
	assert(kn_value_is_ast(args[0]));

	kn_value_free(kn_ast_run(kn_value_as_ast(args[0])));

	return kn_value_run(args[1]);
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
		struct kn_string *string = kn_value_to_string(args[0]);
		variable = kn_env_fetch(kn_string_deref(string), kn_length(string));
		kn_string_free(string);
	}
#endif /* KN_EXT_EQL_INTERPOLATE */

	kn_value ret = kn_value_run(args[1]);
	kn_variable_assign(variable, kn_value_clone(ret));
	return ret;
}

DECLARE_FUNCTION(while, 2, "WHILE") {
	while (kn_value_to_boolean(args[0]))
		kn_value_free(kn_value_run(args[1]));

	return KN_NULL;
}

DECLARE_FUNCTION(if, 3, "IF") {
	return kn_value_run(args[1 + !kn_value_to_boolean(args[0])]);
}

DECLARE_FUNCTION(get, 3, "GET") {
	kn_value container = kn_value_run(args[0]);
	kn_integer start = kn_value_to_integer(args[1]);
	kn_integer length = kn_value_to_integer(args[2]);

	if (start < 0 || start != (kn_integer) (size_t) start)
		kn_error("starting index is negative or too large: %"PRIdkn, start);

	if (length < 0 || length != (kn_integer) (size_t) length)
		kn_error("length is negative or too large: %"PRIdkn, length);

	if (!kn_value_is_string(container) && !kn_value_is_list(container))
		kn_error("can only call GET on lists and strings");

	// NOTE: both strings and lists have the the length in the same spot.
	if ((kn_integer) kn_container_length(container) < start + length) {
		kn_error(
			"invalid bounds for GET: container length = %zu, end index=%"PRIdkn,
			kn_container_length(container),
			start + length
		);
	}

	if (kn_value_is_list(container)) {
		return kn_value_new(kn_list_get_sublist(
			kn_value_as_list(container),
			start,
			length
		));
	}

	return kn_value_new(kn_string_get_substring(
		kn_value_as_string(container),
		start,
		length
	));
}

DECLARE_FUNCTION(set, 4, "SET") {
	kn_value container = kn_value_run(args[0]);
	kn_integer start = kn_value_to_integer(args[1]);
	kn_integer length = kn_value_to_integer(args[2]);

	if (start < 0 || start != (kn_integer) (size_t) start)
		kn_error("starting index is negative or too large: %"PRIdkn, start);

	if (length < 0 || length != (kn_integer) (size_t) length)
		kn_error("length is negative or too large: %"PRIdkn, length);

	if (!kn_value_is_string(container) && !kn_value_is_list(container))
		kn_error("can only call SET on lists and strings");

	if ((kn_integer) kn_container_length(container) < start + length) {
		kn_error(
			"invalid bounds for GET: container length = %zu, end index=%"PRIdkn,
			kn_container_length(container),
			start + length
		);
	}

	if (kn_value_is_list(container)) {
		return kn_value_new(kn_list_set_sublist(
			kn_value_as_list(container),
			start,
			length,
			kn_value_to_list(args[3])
		));
	}

	return kn_value_new(kn_string_set_substring(
		kn_value_as_string(container),
		start,
		length,
		kn_value_to_string(args[3])
	));
}

