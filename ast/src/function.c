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
                         KN_UNDEFINED, kn_value_new_number, kn_value_new_string,
                         kn_value_new_boolean, kn_value_clone, kn_value_free,
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

#ifndef KN_RECKLESS
			if (ferror(stdin))
				die("unable to read line from stdin: %s", strerror(errno));
#endif /* !KN_RECKLESS */
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
		return kn_value_new(&kn_string_empty);
	}

	return kn_value_new(kn_string_new_owned(line, length));
}

DECLARE_FUNCTION(random, 0, "RANDOM") {
	(void) args;
	return kn_value_new((kn_number) rand());
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
	return kn_value_run(args[0]);
}

DECLARE_FUNCTION(box, 1, ",") {
	return kn_value_new(kn_list_box(kn_value_run(args[0])));
}

DECLARE_FUNCTION(head, 1, "[") {
	kn_value ran = kn_value_run(args[0]);

	if (kn_value_is_list(ran)) {
#ifndef KN_RECKLESS
		if (!LIST(ran)->length)
			die("called `[` on an empty list");
#endif /* !KN_RECKLESS */

		kn_value head = kn_value_clone(LIST(ran)->elements[0]);
		kn_list_free(LIST(ran));
		return head;
	}

#ifndef KN_RECKLESS
	if (!kn_value_is_string(ran))
		die("can only call `[` on strings and lists");

	if (!kn_string_length(STRING(ran)))
			die("called `[` on an empty string");
#endif /* !KN_RECKLESS */

	struct kn_string *head = kn_string_new_borrowed(kn_string_deref(STRING(ran)), 1);
	kn_string_free(STRING(ran));

	return kn_value_new(head);
}

DECLARE_FUNCTION(tail, 1, "]") {
	kn_value ran = kn_value_run(args[0]);

	if (kn_value_is_list(ran)) {
		struct kn_list *tail = kn_list_tail(LIST(ran));

#ifndef KN_RECKLESS
		if (!tail)
			die("called `]` on an empty list");
#endif /* !KN_RECKLESS */

		return kn_value_new(tail);
	}

#ifndef KN_RECKLESS
	if (!kn_value_is_string(ran))
		die("can only call `]` on strings and lists");

	if (!kn_string_length(STRING(ran)))
			die("called `]` on an empty string");
#endif /* !KN_RECKLESS */

	struct kn_string *tail = kn_string_new_borrowed(
		kn_string_deref(STRING(ran)) + 1,
		kn_string_length(STRING(ran)) - 1
	);
	kn_string_free(STRING(ran));

	return kn_value_new(tail);
}

DECLARE_FUNCTION(block, 1, "BLOCK") {
	assert(kn_value_is_ast(args[0]));
	kn_ast_clone(AST(args[0]));
	return args[0];
}

DECLARE_FUNCTION(call, 1, "CALL") {
	kn_value ran = kn_value_run(args[0]);

#ifndef KN_RECKLESS
	if (!kn_value_is_ast(ran))
		die("can only CALL `BLOCK`s.");
#endif /* !KN_RECKLESS */

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
	struct kn_string *command = kn_value_to_string(args[0]);

	if (kn_value_run(args[1]) != KN_NULL)
		die("only `NULL` for a second arg is currently supported");

	const char *str = kn_string_deref(command);
	FILE *stream = popen(str, "r");

#ifndef KN_RECKLESS
	if (stream == NULL)
		die("unable to execute command '%s'.", str);
#endif /* !KN_RECKLESS */

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

#ifndef KN_RECKLESS
	// Abort if `stream` had an error.
	if (ferror(stream))
		die("unable to read command stream");
#endif /* !KN_RECKLESS */

	result = xrealloc(result, length + 1);
	result[length] = '\0';

#ifndef KN_RECKLESS
	// Abort if we cant close stream.
	if (pclose(stream) == -1)
		die("unable to close command stream");
#endif /* !KN_RECKLESS */

	return kn_value_new(kn_string_new_owned(result, length));
}
#endif /* KN_EXT_SYSTEM */

DECLARE_FUNCTION(quit, 1, "QUIT") {
	exit((int) to_number(args[0]));
}

DECLARE_FUNCTION(not, 1, "!") {
	return kn_value_new((kn_boolean) !to_boolean(args[0]));
}

DECLARE_FUNCTION(length, 1, "LENGTH") {
	kn_value ran = kn_value_run(args[0]);

	if (kn_value_is_string(ran)) {
		kn_number length = (kn_number) kn_string_length(STRING(ran));
		kn_string_free(STRING(ran));
		return kn_value_new(length);
	}

	if (kn_value_is_list(ran)) {
		kn_number length = (kn_number) LIST(ran)->length;
		kn_list_free(LIST(ran));
		return kn_value_new(length);
	}

	if (kn_value_is_number(ran)) {
		kn_number num = NUMBER(ran);
		kn_number length = 0;
		do {
			length++;
			num /= 10;
		} while (num);
		return kn_value_new(length);
	}

	struct kn_list *list = to_list(ran);
	kn_value_free(ran);
	kn_number length = (kn_number) list->length;
	kn_list_free(list);
	return kn_value_new(length);
}

DECLARE_FUNCTION(dump, 1, "DUMP") {
	kn_value ret = kn_value_run(args[0]);

	kn_value_dump(ret, stdout);
	putchar('\n');

	return ret;
}

DECLARE_FUNCTION(output, 1, "OUTPUT") {
	struct kn_string *string = to_string(args[0]);
	size_t length = kn_string_length(string);
	char *str = kn_string_deref(string);
	char *penult;

#ifndef KN_RECKLESS
	clearerr(stdout);
#endif /* !KN_RECKLESS */

	if (length == 0) {
		putc('\n', stdout);
	} else if (KN_UNLIKELY('\\' == *(penult = &str[length - 1]))) {
		*penult = '\0'; // replace the trailing `\`, so it wont be printed...
		fputs(str, stdout);
		*penult = '\\'; // ...and then restore it.
		fflush(stdout);
	} else {
		puts(str);
	}

#ifndef KN_RECKLESS
	if (ferror(stdout))
		die("unable to write to stdout");
#endif /* !KN_RECKLESS */

	kn_string_free(string);
	return KN_NULL;
}

DECLARE_FUNCTION(ascii, 1, "ASCII") {
	kn_value value = kn_value_run(args[0]);

	// If lhs is a string, convert both to a string and concatenate.
	if (kn_value_is_string(value)) {
#ifndef KN_RECKLESS
		if (!kn_string_length(STRING(value)))
			die("ASCII called with empty string.");
#endif /* !KN_RECKLESS */
		return kn_value_new((kn_number) *kn_string_deref(STRING(value)));
	}

#ifndef KN_RECKLESS
	if (!kn_value_is_number(value))
		die("can only call ASCII on numbers and strings.");

	// just check for ASCIIness, not actually full-blown knight correctness. 
	if (NUMBER(value) < 0 || 127 < NUMBER(value))
		die("Numeric value %" PRIdkn " is out of range for ascii char.", NUMBER(value));
#endif

	char buf[2] = { NUMBER(value), 0 };
	return kn_value_new(kn_string_new_borrowed(buf, 1));
}

#ifdef KN_EXT_VALUE
DECLARE_FUNCTION(value, 1, "VALUE") {
	struct kn_string *string = to_string(args[0]);
	struct kn_variable *variable = kn_env_fetch(kn_string_deref(string), kn_string_length(string));

	kn_string_free(string);
	return kn_variable_run(variable);
}
#endif /* KN_EXT_VALUE */

DECLARE_FUNCTION(negate, 1, "~") {
	return kn_value_new((kn_number) -to_number(args[0]));
}

DECLARE_FUNCTION(add, 2, "+") {
	kn_value lhs = kn_value_run(args[0]);

	switch (kn_tag(args[0])) {
	case KN_TAG_LIST:
		return kn_value_new(kn_list_concat(LIST(lhs), to_list(args[1])));

	case KN_TAG_STRING:
		return kn_value_new(kn_string_concat(STRING(lhs), to_string(args[1])));

	case KN_TAG_NUMBER:
		return kn_value_new(NUMBER(lhs) + to_number(args[1]));

	default:
#ifndef KN_RECKLESS
		die("can only add to strings, numbers, and lists");
#endif /* !KN_RECKLESS */
		KN_UNREACHABLE();
	}

}

DECLARE_FUNCTION(sub, 2, "-") {
	kn_value lhs = kn_value_run(args[0]);

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only subtract from numbers");
#endif /* !KN_RECKLESS */

	return kn_value_new(NUMBER(lhs) - to_number(args[1]));
}

DECLARE_FUNCTION(mul, 2, "*") {
	kn_value lhs = kn_value_run(args[0]);
	kn_number rhs = to_number(args[1]);

	if (kn_value_is_number(lhs))
		return kn_value_new(NUMBER(lhs) * rhs);

#ifndef KN_RECKLESS
	if (rhs < 0 || rhs != (kn_number) (unsigned) rhs)
		die("negative or too large an amount given");

	if (!kn_value_is_list(lhs) && !kn_value_is_string(lhs))
		die("can only multiply numbers, strings, and lists");
#endif /* !KN_RECKLESS */

	// If lhs is a string, convert rhs to a number and multiply.
	if (kn_value_is_string(lhs))
		return kn_value_new(kn_string_repeat(STRING(lhs), rhs));

	return kn_value_new(kn_list_repeat(LIST(lhs), rhs));
}

DECLARE_FUNCTION(div, 2, "/") {
	kn_value lhs = kn_value_run(args[0]);

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only divide numbers");
#endif /* !KN_RECKLESS */

	kn_number divisor = to_number(args[1]);

#ifndef KN_RECKLESS
	if (!divisor)
		die("attempted to divide by zero");
#endif /* !KN_RECKLESS */

	return kn_value_new(NUMBER(lhs) / divisor);
}

DECLARE_FUNCTION(mod, 2, "%") {
	kn_value lhs = kn_value_run(args[0]);

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only modulo numbers");
#endif /* !KN_RECKLESS */

	kn_number base = to_number(args[1]);

#ifndef KN_RECKLESS
	if (!base)
		die("attempted to modulo by zero");
#endif /* !KN_RECKLESS */

	return kn_value_new(NUMBER(lhs) % base);
}

DECLARE_FUNCTION(pow, 2, "^") {
	kn_value lhs = kn_value_run(args[0]);

	if (kn_value_is_list(lhs)) {
		struct kn_string *sep = to_string(args[1]);
		struct kn_string *joined = kn_list_join(LIST(lhs), sep);

		kn_list_free(LIST(lhs));
		kn_string_free(sep);

		return kn_value_new(joined);
	}

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only exponentiate numbers and lists");
#endif /* !KN_RECKLESS */

	return kn_value_new((kn_number) powl(NUMBER(lhs), to_number(args[1])));
}

DECLARE_FUNCTION(eql, 2, "?") {
	kn_value lhs = kn_value_run(args[0]);
	kn_value rhs = kn_value_run(args[1]);

	kn_boolean eql = kn_value_equal(lhs, rhs);

	kn_value_free(lhs);
	kn_value_free(rhs);

	return kn_value_new(eql);
}

DECLARE_FUNCTION(lth, 2, "<") {
	kn_value lhs = kn_value_run(args[0]);
	kn_value rhs = kn_value_run(args[1]);

	kn_boolean less = kn_value_compare(lhs, rhs) < 0;

	kn_value_free(lhs);
	kn_value_free(rhs);

	return kn_value_new(less);
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

	// return the lhs if its falsey.
	if (!to_boolean(lhs))
		return lhs;

	kn_value_free(lhs);
	return kn_value_run(args[1]);
}

DECLARE_FUNCTION(or, 2, "|") {
	kn_value lhs = kn_value_run(args[0]);

	// return the lhs if its truthy.
	if (to_boolean(lhs))
		return lhs;

	kn_value_free(lhs);
	return kn_value_run(args[1]);
}

DECLARE_FUNCTION(then, 2, ";") {
	// We ensured the first value was an ast in the parser.
	assert(kn_value_is_ast(args[0]));

	kn_value_free(kn_ast_run(AST(args[0])));
	return kn_value_run(args[1]);
}

DECLARE_FUNCTION(assign, 2, "=") {
	struct kn_variable *variable;

#ifdef KN_EXT_EQL_INTERPOLATE
	// if it's an identifier, special-case it where we don't evaluate it.
	if (KN_LIKELY(kn_value_is_variable(args[0]))) {
#else
#	ifndef KN_RECKLESS
		if (!kn_value_is_variable(args[0]))
			die("can only assign to variables");
#	endif /* KN_RECKLESS */
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


	kn_value ret = kn_value_run(args[1]);
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
		kn_value_free(kn_value_run(args[1]));
	return KN_NULL;
}

DECLARE_FUNCTION(if, 3, "IF") {
	return kn_value_run(args[1 + !to_boolean(args[0])]);
}

DECLARE_FUNCTION(get, 3, "GET") {
	kn_value container = kn_value_run(args[0]);
	kn_number start = to_number(args[1]);
	kn_number length = to_number(args[2]);

#ifndef KN_RECKLESS
	if (start < 0 || start != (kn_number) (unsigned) start)
		die("starting index is negative or too large: %"PRIdkn, start);

	if (length < 0 || length != (kn_number) (unsigned) length)
		die("length is negative or too large: %"PRIdkn, length);

	if (!kn_value_is_string(container) && !kn_value_is_list(container))
		die("can only call GET on lists and strings");

	// NOTE: both strings and lists have the the length in the same spot.
	unsigned container_length = ((struct kn_list *) KN_UNMASK(container))->length;

	if (container_length < start + length) {
		die("invalid bounds given to GET: container length = %u, start+length=%"PRIdkn,
			container_length, start + length);
	}
#endif /* !KN_RECKLESS */

	if (kn_value_is_list(container)) {
		struct kn_list *sublist = kn_list_sublist(LIST(container), start, length);
		kn_list_free(LIST(container));
		return kn_value_new(sublist);
	}

	struct kn_string *substring = kn_string_substring(STRING(container), start, length);
	kn_string_free(STRING(container));
	return kn_value_new(substring);
}

DECLARE_FUNCTION(set, 4, "SET") {
	struct kn_string *string, *replacement, *result;
	size_t start, amnt, string_length, replacement_length;

	string = kn_value_to_string(args[0]);
	start = (size_t) kn_value_to_number(args[1]);
	amnt = (size_t) kn_value_to_number(args[2]);
	replacement = kn_value_to_string(args[3]);

	string_length = kn_string_length(string);
	replacement_length = kn_string_length(replacement);

#ifndef KN_RECKLESS
	// if it's out of bounds, die.
	if (string_length < start)
		die("index '%zu' out of bounds (length=%zu)", start, string_length);
#endif /* !KN_RECKLESS */

	if (string_length <= start + amnt) {
		amnt = string_length - start;
	}

	if (start == 0 && replacement_length == 0) {
		result = kn_string_new_borrowed(kn_string_deref(string) + amnt, string_length - amnt);
		kn_string_free(string);
		return kn_value_new_string(result);
	}

	// TODO: you could also check for caching here first.
	result = kn_string_alloc(string_length - amnt + replacement_length);
	char *ptr = kn_string_deref(result);

	memcpy(ptr, kn_string_deref(string), start);
	ptr += start;

	memcpy(ptr, kn_string_deref(replacement), replacement_length);
	ptr += replacement_length;
	kn_string_free(replacement);

	memcpy(
		ptr,
		kn_string_deref(string) + start + amnt,
		string_length - amnt - start + 1 // `+1` so we copy the `\0` too.
	);

	kn_string_free(string);
	kn_string_cache(result);

	return kn_value_new_string(result);
}

