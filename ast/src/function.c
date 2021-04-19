#include "function.h" /* prototypes */
#include "knight.h"   /* kn_run */
#include "env.h"      /* kn_env_fetch, kn_variable, kn_variable_run,
                         kn_variable_assign */
#include "shared.h"   /* die, xmalloc, xrealloc, kn_hash, kn_hash_acc,
                         KN_LIKELY, KN_UNLIKELY */
#include "string.h"   /* kn_string, kn_string_new_owned, kn_string_new_borrowed,
                         kn_string_alloc, kn_string_free, kn_string_empty,
                         kn_string_deref, kn_string_length, kn_string_cache,
                         kn_string_clone_static, kn_string_cache_lookup */
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

void kn_function_startup(void) {
	// all we have to do on startup is seed the random number.
	srand(time(NULL));
}

#define DECLARE_FUNCTION(func, arity, name) \
	static KN_DECLARE_FUNCTION(kn_fn_##func, arity, name)

DECLARE_FUNCTION(prompt, 0, "PROMPT") {
	(void) args;

	size_t capacity = 0;
	ssize_t length; // todo: remove the ssize_t
	char *line = NULL;

	// TODO: use fgets instead

	// try to read a line from stdin.
	if ((length = getline(&line, &capacity, stdin)) == -1) {
		assert(line != NULL);
		free(line);

#ifndef KN_RECKLESS
		// if we're not at eof, abort.
		if (KN_UNLIKELY(!feof(stdin)))
			die("unable to read line");
#endif /* !KN_RECKLESS */

		return kn_value_new_string(&kn_string_empty);
	}

	assert(0 < length);
	assert(line != NULL);

	if (KN_LIKELY(length-- != 0 && line[length] == '\n')) {
		if (KN_LIKELY(length != 0) && line[length - 1] == '\r')
			length--;
	} else {
		++length; // as we subtracted it earlier preemptively.
	}

	struct kn_string *string =
		KN_UNLIKELY(length == 0)
			? &kn_string_empty
			: kn_string_new_owned(strndup(line, length), length);

	free(line);

	return kn_value_new_string(string);
}

DECLARE_FUNCTION(random, 0, "RANDOM") {
	(void) args;

	return kn_value_new_number((kn_number) rand());
}

DECLARE_FUNCTION(eval, 1, "EVAL") {
	struct kn_string *string = kn_value_to_string(args[0]);
	kn_value ret = kn_run(kn_string_deref(string));

	kn_string_free(string);

	return ret;
}

DECLARE_FUNCTION(block, 1, "BLOCK") {
	return kn_value_clone(args[0]);
}

DECLARE_FUNCTION(call, 1, "CALL") {
	kn_value ran = kn_value_run(args[0]);
	kn_value result = kn_value_run(ran);

	kn_value_free(ran);

	return result;
}

DECLARE_FUNCTION(system, 1, "`") {
	struct kn_string *command = kn_value_to_string(args[0]);
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

	return kn_value_new_string(kn_string_new_owned(result, length));
}

DECLARE_FUNCTION(quit, 1, "QUIT") {
	exit((int) kn_value_to_number(args[0]));
}

DECLARE_FUNCTION(not, 1, "!") {
	return kn_value_new_boolean(!kn_value_to_boolean(args[0]));
}

DECLARE_FUNCTION(length, 1, "LENGTH") {
	struct kn_string *string = kn_value_to_string(args[0]);
	size_t length = kn_string_length(string);

	kn_string_free(string);

	return kn_value_new_number((kn_number) length);
}

DECLARE_FUNCTION(dump, 1, "DUMP") {
	kn_value ret = kn_value_run(args[0]);

	kn_value_dump(ret);

	putc('\n', stdout);

	return ret;
}

DECLARE_FUNCTION(output, 1, "OUTPUT") {
	struct kn_string *string = kn_value_to_string(args[0]);
	size_t length = kn_string_length(string);
	char *str = kn_string_deref(string);
	char *penult;

#ifndef KN_RECKLESS
	clearerr(stdout);
#endif /* !KN_RECKLESS */

	if (length == 0) {
		putc('\n', stdout);
	} else if (KN_UNLIKELY('\\' == *(penult = &str[length - 1]))) {
		*penult = '\0'; // replace the trailing `\`, so it wont be printed
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

#ifdef KN_EXT_VALUE
DECLARE_FUNCTION(value, 1, "VALUE") {
	struct kn_string *string = kn_value_to_string(args[0]);
	struct kn_variable *variable = kn_env_fetch(kn_string_deref(string));

	kn_string_free(string);
	return kn_variable_run(variable);
}
#endif /* KN_EXT_VALUE */

#ifdef KN_EXT_NEGATE
DECLARE_FUNCTION(negate, 1, "~") {
	return kn_value_new_number(-kn_value_to_number(args[0]));
}
#endif /* KN_EXT_NEGATE */

static kn_value add_string(struct kn_string *lhs, struct kn_string *rhs) {
	size_t lhslen, rhslen;

	// return early if either
	if ((lhslen = kn_string_length(lhs)) == 0) {
		assert(lhs == &kn_string_empty);
		return kn_value_new_string(kn_string_clone_static(rhs));
	}

	if ((rhslen = kn_string_length(rhs)) == 0) {
		assert(rhs == &kn_string_empty);
		return kn_value_new_string(lhs);
	}

	unsigned long hash = kn_hash(kn_string_deref(lhs), kn_string_length(lhs));
	hash = kn_hash_acc(kn_string_deref(rhs), kn_string_length(rhs), hash);

	size_t length = lhslen + rhslen;

	struct kn_string *string = kn_string_cache_lookup(hash, length);
	if (string == NULL)
		goto allocate_and_cache;

	char *cached = kn_string_deref(string);
	char *tmp = kn_string_deref(lhs);

	for (size_t i = 0; i < kn_string_length(lhs); i++, cached++)
		if (*cached != tmp[i])
			goto allocate_and_cache;

	tmp = kn_string_deref(rhs);

	for (size_t i = 0; i < kn_string_length(rhs); i++, cached++)
		if (*cached != tmp[i])
			goto allocate_and_cache;

	string = kn_string_clone(string);
	goto free_and_return;

allocate_and_cache:
	string = kn_string_alloc(length);
	char *str = kn_string_deref(string);

	memcpy(str, kn_string_deref(lhs), lhslen);
	memcpy(str + lhslen, kn_string_deref(rhs), rhslen);
	str[length] = '\0';

	kn_string_cache(string);
free_and_return:

	kn_string_free(lhs);
	kn_string_free(rhs);

	return kn_value_new_string(string);
}

DECLARE_FUNCTION(add, 2, "+") {
	kn_value lhs = kn_value_run(args[0]);

	// If lhs is a string, convert both to a string and concatenate.
	if (kn_value_is_string(lhs))
		return add_string(kn_value_as_string(lhs), kn_value_to_string(args[1]));

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only add to strings and numbers");
#endif /* !KN_RECKLESS */

	kn_number augend = kn_value_as_number(lhs);
	kn_number addend = kn_value_to_number(args[1]);

	return kn_value_new_number(augend + addend);
}

DECLARE_FUNCTION(sub, 2, "-") {
	kn_value lhs = kn_value_run(args[0]);

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only subtract from numbers");
#endif /* !KN_RECKLESS */

	kn_number minuend = kn_value_as_number(lhs);
	kn_number subtrahend = kn_value_to_number(args[1]);

	return kn_value_new_number(minuend - subtrahend);
}

static kn_value mul_string(struct kn_string *lhs, size_t times) {
	size_t lhslen = kn_string_length(lhs);

	if (lhslen == 0 || times == 0) {

		// if the string is not empty, free it.
		if (lhslen != 0) {
			kn_string_free(lhs);
		} else {
			assert(lhs == &kn_string_empty);
		}

		return kn_value_new_string(&kn_string_empty);
	}

	// we don't have to clone it, as we were given the cloned copy.
	if (times == 1)
		return kn_value_new_string(lhs);

	size_t length = lhslen * times;
	struct kn_string *string = kn_string_alloc(length);
	char *str = kn_string_deref(string);

	for (char *ptr = str; times != 0; --times, ptr += lhslen)
		memcpy(ptr, kn_string_deref(lhs), lhslen);

	str[length] = '\0';

	kn_string_free(lhs);

	return kn_value_new_string(string);
}

DECLARE_FUNCTION(mul, 2, "*") {
	kn_value lhs = kn_value_run(args[0]);

	// If lhs is a string, convert rhs to a number and multiply.
	if (kn_value_is_string(lhs)) {
		kn_number amnt = kn_value_to_number(args[1]);

#ifndef KN_RECKLESS
		if (amnt < 0 || amnt != (kn_number) (size_t) amnt)
			die("negative or too large an amount given");
#endif /* !KN_RECKLESS */

		return mul_string(kn_value_as_string(lhs), amnt);
	}

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only multiply with numbers and strings");
#endif /* !KN_RECKLESS */

	kn_number multiplicand = kn_value_as_number(lhs);
	kn_number multiplier = kn_value_to_number(args[1]);

	return kn_value_new_number(multiplicand * multiplier);
}

DECLARE_FUNCTION(div, 2, "/") {
	kn_value lhs = kn_value_run(args[0]);

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only divide numbers");
#endif /* !KN_RECKLESS */

	kn_number dividend = kn_value_as_number(lhs);
	kn_number divisor = kn_value_to_number(args[1]);

#ifndef KN_RECKLESS
	if (divisor == 0)
		die("attempted to divide by zero");
#endif /* !KN_RECKLESS */

	return kn_value_new_number(dividend / divisor);
}

DECLARE_FUNCTION(mod, 2, "%") {
	kn_value lhs = kn_value_run(args[0]);

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only modulo numbers");
#endif /* !KN_RECKLESS */

	kn_number number = kn_value_as_number(lhs);
	kn_number base = kn_value_to_number(args[1]);

#ifndef KN_RECKLESS
	if (base == 0)
		die("attempted to modulo by zero");
#endif /* !KN_RECKLESS */

	return kn_value_new_number(number % base);
}

DECLARE_FUNCTION(pow, 2, "^") {
	kn_value lhs = kn_value_run(args[0]);

#ifndef KN_RECKLESS
	if (!kn_value_is_number(lhs))
		die("can only exponentiate numbers: %lld", lhs);
#endif /* !KN_RECKLESS */

	kn_number result;
	kn_number base = kn_value_as_number(lhs);
	kn_number exponent = kn_value_to_number(args[1]);

	// there's no builtin way to do integer exponentiation, so we have to
	// do it manually.
	if (base == 1) result = 1;
	else if (base == -1) result = exponent & 1 ? -1 : 1;
	else if (exponent == 1) result = base;
	else if (exponent == 0) result = 1;
	else if (exponent < 0) result = 0; // already handled `base == -1`
	else {
		for (result = 1; exponent > 0; --exponent)
			result *= base;
	}

	return kn_value_new_number(result);
}

DECLARE_FUNCTION(eql, 2, "?") {
	kn_value lhs = kn_value_run(args[0]);
	kn_value rhs = kn_value_run(args[1]);
	bool eql;

	assert(lhs != KN_UNDEFINED);
	assert(rhs != KN_UNDEFINED);

	if ((eql = (lhs == rhs)))
		goto free_and_return;

	if (!(eql = (kn_value_is_string(lhs) && kn_value_is_string(rhs))))
		goto free_and_return;

	struct kn_string *lstr = kn_value_as_string(lhs);
	struct kn_string *rstr = kn_value_as_string(rhs);
	size_t llen = kn_string_length(lstr);

	eql = llen == kn_string_length(rstr) &&
		!memcmp(kn_string_deref(lstr), kn_string_deref(rstr), llen);

free_and_return:

	kn_value_free(lhs);
	kn_value_free(rhs);

	return kn_value_new_boolean(eql);
}

DECLARE_FUNCTION(lth, 2, "<") {
	kn_value lhs = kn_value_run(args[0]);
	bool less;

	if (kn_value_is_string(lhs)) {
		struct kn_string *lstr = kn_value_as_string(lhs);
		struct kn_string *rstr = kn_value_to_string(args[1]);

		less = strcmp(kn_string_deref(lstr), kn_string_deref(rstr)) < 0;

		kn_string_free(lstr);
		kn_string_free(rstr);
	} else if (kn_value_is_number(lhs)) {
		less = kn_value_as_number(lhs) < kn_value_to_number(args[1]);
	} else {
#ifndef KN_RECKLESS
		if (!kn_value_is_boolean(lhs))
			die("can only compare to numbers, strings, and booleans");
#endif /* KN_RECKLESS */

		// note that `== KN_FALSE` needs to be after, otherwise rhs wont be run.
		less = kn_value_to_boolean(args[1]) && lhs == KN_FALSE;
	}

	return kn_value_new_boolean(less);
}

DECLARE_FUNCTION(gth, 2, ">") {
	kn_value lhs = kn_value_run(args[0]);
	bool more;

	if (kn_value_is_string(lhs)) {
		struct kn_string *lstr = kn_value_as_string(lhs);
		struct kn_string *rstr = kn_value_to_string(args[1]);

		more = strcmp(kn_string_deref(lstr), kn_string_deref(rstr)) > 0;

		kn_string_free(lstr);
		kn_string_free(rstr);
	} else if (kn_value_is_number(lhs)) {
		more = kn_value_as_number(lhs) > kn_value_to_number(args[1]);
	} else {
#ifndef KN_RECKLESS
		if (!kn_value_is_boolean(lhs))
			die("can only compare to numbers, strings, and booleans");
#endif /* KN_RECKLESS */

		// note that `== KN_TRUE` needs to be after, otherwise rhs wont be run.
		more = !kn_value_to_boolean(args[1]) && lhs == KN_TRUE;
	}

	return kn_value_new_boolean(more);
}

DECLARE_FUNCTION(and, 2, "&") {
	kn_value lhs = kn_value_run(args[0]);

	// return the lhs if its falsey.
	if (!kn_value_to_boolean(lhs))
		return lhs;

	kn_value_free(lhs);
	return kn_value_run(args[1]);
}

DECLARE_FUNCTION(or, 2, "|") {
	kn_value lhs = kn_value_run(args[0]);

	// return the lhs if its truthy.
	if (kn_value_to_boolean(lhs))
		return lhs;

	kn_value_free(lhs);
	return kn_value_run(args[1]);
}

DECLARE_FUNCTION(then, 2, ";") {
	kn_value_free(kn_value_run(args[0]));

	return kn_value_run(args[1]);
}

DECLARE_FUNCTION(assign, 2, "=") {
	struct kn_variable *variable;
	kn_value ret;

#ifdef KN_EXT_EQL_INTERPOLATE
	// if it's an identifier, special-case it where we don't evaluate it.
	if (KN_LIKELY(kn_value_is_variable(args[0]))) {
#endif /* KN_EXT_EQL_INTERPOLATE */

	variable = kn_value_as_variable(args[0]);
	ret = kn_value_run(args[1]);

#ifdef KN_EXT_EQL_INTERPOLATE
	} else {
		// otherwise, evaluate the expression, convert to a string,
		// and then use that as the variable.
		variable = kn_env_fetch(kn_string_deref(kn_value_to_string(args[0])));
		ret = kn_value_run(args[1]);
	}
#endif /* KN_EXT_EQL_INTERPOLATE */

	kn_variable_assign(variable, kn_value_clone(ret));

	return ret;
}

DECLARE_FUNCTION(while, 2, "WHILE") {
	while (kn_value_to_boolean(args[0]))
		kn_value_free(kn_value_run(args[1]));

	return KN_NULL;
}

DECLARE_FUNCTION(if, 3, "IF") {
	bool idx = kn_value_to_boolean(args[0]);

	return kn_value_run(args[1 + !idx]);
}

DECLARE_FUNCTION(get, 3, "GET") {
	struct kn_string *string, *substring;
	size_t start, length, string_length;

	string = kn_value_to_string(args[0]);
	start = (size_t) kn_value_to_number(args[1]);
	length = (size_t) kn_value_to_number(args[2]);

	string_length = kn_string_length(string);

	// if we're getting past the end of the array, simply return the
	// empty string.
	if (KN_UNLIKELY(string_length <= start)) {
		substring = &kn_string_empty;
	} else {
		// if the total length is too much, simply wrap around to the end.
#ifndef KN_RECKLESS
		if (string_length < start + length)
			die("ending position is too large!");
#endif /* KN_RECKLESS */

		substring =
			kn_string_new_borrowed(kn_string_deref(string) + start, length);
	}

	kn_string_free(string);

	return kn_value_new_string(substring);
}

DECLARE_FUNCTION(substitute, 4, "SUBSTITUTE") {
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

