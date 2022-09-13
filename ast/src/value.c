#include "env.h"    /* kn_variable, kn_variable_run */
#include "ast.h"    /* kn_ast, kn_ast_deallocate, kn_ast_clone, kn_ast_run */
#include "value.h"  /* prototypes, bool, uint64_t, int64_t, kn_value, kn_number,
                       kn_boolean, KN_UNDEFINED, KN_NULL, KN_TRUE, KN_FALSE */
#include "string.h" /* kn_string, kn_string_clone, kn_string_deallocate,
                       kn_string_deref, kn_string_length, KN_STRING_FL_STATIC,
                       KN_STRING_NEW_EMBED */
#include "custom.h" /* kn_custom, kn_custom_free, kn_custom_clone */
#include "shared.h" /* die */
#include "number.h" /* kn_number_to_string */
#include "list.h"

#include <inttypes.h> /* PRId64 */
#include <stdlib.h>   /* free, NULL */
#include <assert.h>   /* assert */
#include <stdio.h>    /* printf */
#include <ctype.h>    /* isspace */

#define KN_REFCOUNT(x) ((unsigned *) KN_UNMASK(x))


/*
 * Convert a string to a number, as per the knight specs.
 *
 * This means we strip all leading whitespace, and then an optional `-` or `+`
 * may appear (`+` is ignored, `-` indicates a negative number). Then as many
 * digits as possible are read.
 *
 * Note that we can't use `strtoll`, as we can't be positive that `kn_number`
 * is actually a `long long`.
 */
static kn_number string_to_number(struct kn_string *string) {
	kn_number ret = 0;
	const char *ptr = kn_string_deref(string);

	// strip leading whitespace.
	while (KN_UNLIKELY(isspace(*ptr)))
		ptr++;

	bool is_neg = *ptr == '-';

	// remove leading `-` or `+`s, if they exist.
	if (is_neg || *ptr == '+')
		++ptr;

	// only digits are `<= 9` when a literal `0` char is subtracted from them.
	unsigned char cur; // be explicit about wraparound.
	while ((cur = *ptr++ - '0') <= 9)
		ret = ret * 10 + cur;

	return is_neg ? -ret : ret;
}

kn_number kn_value_to_number(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (KN_EXPECT(kn_tag(value), KN_TAG_NUMBER)) {
	case KN_TAG_NUMBER:
		return kn_value_as_number(value);

	case KN_TAG_CONSTANT:
		return value == KN_TRUE;

	case KN_TAG_STRING:
		return string_to_number(kn_value_as_string(value));

	case KN_TAG_LIST:
		return kn_value_as_list(value)->length;

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_number != NULL)
			return custom->vtable->to_number(custom->data);
		// otherwise, fallthrough
	}
#endif /* KN_CUSTOM */

	case KN_TAG_VARIABLE:
	case KN_TAG_AST: {
		// simply execute the value and call this function again.
		kn_value ran = kn_value_run(value);
		kn_number ret = kn_value_to_number(ran);
		kn_value_free(ran);
		return ret;
	}

	default:
		KN_UNREACHABLE();
	}
}

kn_boolean kn_value_to_boolean(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (kn_tag(value)) {
	case KN_TAG_CONSTANT:
		return value == KN_TRUE;

	case KN_TAG_NUMBER:
		return value != KN_TAG_NUMBER;

	case KN_TAG_STRING:
		return kn_string_length(kn_value_as_string(value)) != 0;

	case KN_TAG_LIST:
		return kn_value_as_list(value)->length != 0;

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_boolean != NULL)
			return custom->vtable->to_boolean(custom->data);
		// otherwise, fallthrough
	}
#endif /* KN_CUSTOM */

	case KN_TAG_AST:
	case KN_TAG_VARIABLE: {
		// simply execute the value and call this function again.
		kn_value ran = kn_value_run(value);
		kn_boolean ret = kn_value_to_boolean(ran);
		kn_value_free(ran);

		return ret;
	}

	default:
		KN_UNREACHABLE();
	}
}

struct kn_string *kn_value_to_string(kn_value value) {
	// static, embedded strings so we don't have to allocate for known strings.
	static struct kn_string newline = KN_STRING_NEW_EMBED("\n");
	static struct kn_string builtin_strings[KN_TRUE + 1] = {
		[KN_FALSE] = KN_STRING_NEW_EMBED("false"),
		[KN_TAG_NUMBER] = KN_STRING_NEW_EMBED("0"),
		[KN_NULL] = KN_STRING_NEW_EMBED("null"),
		[KN_TRUE] = KN_STRING_NEW_EMBED("true"),
		[(((uint64_t) 1) << KN_SHIFT) | KN_TAG_NUMBER] = KN_STRING_NEW_EMBED("1"),
	};

	assert(value != KN_UNDEFINED);

	if (KN_UNLIKELY(value <= KN_TRUE))
		return &builtin_strings[value];

	switch (KN_EXPECT(kn_tag(value), KN_TAG_STRING)) {
	case KN_TAG_NUMBER:
		return kn_number_to_string(kn_value_as_number(value));

	case KN_TAG_STRING:
		++*KN_REFCOUNT(value);

		return kn_value_as_string(value);

	case KN_TAG_LIST:
		return kn_list_join(kn_value_as_list(value), &newline);

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_string != NULL)
			return custom->vtable->to_string(custom->data);
		// otherwise, fallthrough
	}
#endif /* KN_CUSTOM */

	case KN_TAG_AST:
	case KN_TAG_VARIABLE: {
		// simply execute the value and call this function again.
		kn_value ran = kn_value_run(value);
		struct kn_string *ret = kn_value_to_string(ran);
		kn_value_free(ran);

		return ret;
	}

	case KN_TAG_CONSTANT:
	default:
		KN_UNREACHABLE();
	}
}


struct kn_list *kn_value_to_list(kn_value value) {
	(void) value;
	die("todo: kn value to list");
// 	// static, embedded strings so we don't have to allocate for known strings.
// 	static struct kn_string builtin_strings[KN_TRUE + 1] = {
// 		[KN_FALSE] = KN_STRING_NEW_EMBED("false"),
// 		[KN_TAG_NUMBER] = KN_STRING_NEW_EMBED("0"),
// 		[KN_NULL] = KN_STRING_NEW_EMBED("null"),
// 		[KN_TRUE] = KN_STRING_NEW_EMBED("true"),
// 		[(((uint64_t) 1) << KN_SHIFT) | KN_TAG_NUMBER] = KN_STRING_NEW_EMBED("1"),
// 	};

// 	assert(value != KN_UNDEFINED);

// 	if (KN_UNLIKELY(value <= KN_TRUE))
// 		return &builtin_strings[value];

// 	switch (KN_EXPECT(kn_tag(value), KN_TAG_STRING)) {
// 	case KN_TAG_NUMBER:
// 		return number_to_string(kn_value_as_number(value));

// 	case KN_TAG_STRING:
// 		++*KN_REFCOUNT(value);

// 		return kn_value_as_string(value);

// #ifdef KN_CUSTOM
// 	case KN_TAG_CUSTOM: {
// 		struct kn_custom *custom = kn_value_as_custom(value);

// 		if (custom->vtable->to_string != NULL)
// 			return custom->vtable->to_string(custom->data);
// 		// otherwise, fallthrough
// 	}
// #endif /* KN_CUSTOM */

// 	case KN_TAG_AST:
// 	case KN_TAG_VARIABLE: {
// 		// simply execute the value and call this function again.
// 		kn_value ran = kn_value_run(value);
// 		struct kn_string *ret = kn_value_to_string(ran);
// 		kn_value_free(ran);

// 		return ret;
// 	}

// 	case KN_TAG_CONSTANT:
// 	default:
// 		KN_UNREACHABLE();
// 	}
}


void kn_value_dump(kn_value value) {
	switch (kn_tag(value)) {
	case KN_TAG_CONSTANT:
		switch (value) {
		case KN_TRUE:  fputs("Boolean(true)", stdout); return;
		case KN_FALSE: fputs("Boolean(false)", stdout); return;
		case KN_NULL:  fputs("Null()", stdout); return;
#ifndef NDEBUG // we dump undefined only for debugging.
		case KN_UNDEFINED: fputs("<KN_UNDEFINED>", stdout); return;
#endif /* !NDEBUG */

		default:
			KN_UNREACHABLE();
		}

	case KN_TAG_NUMBER:
		printf("Number(%" PRId64 ")", kn_value_as_number(value));
		return;

	case KN_TAG_STRING:
		printf("String(%s)", kn_string_deref(kn_value_as_string(value)));
		return;

	case KN_TAG_LIST:
		fputs("List(", stdout);
		struct kn_list *list = kn_value_as_list(value);
		for (unsigned i = 0; i < list->length; ++i) {
			if (i) fputs(", ", stdout);
			kn_value_dump(list->elements[i]);
		}
		fputc(')', stdout);
		return;

	case KN_TAG_VARIABLE:
		printf("Identifier(%s)", kn_value_as_variable(value)->name);
		return;

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->dump != NULL) {
			custom->vtable->dump(custom->data);
		} else {
			printf(
				"Custom(%p, %p)", (void *) custom->data, (void *) custom->vtable
			);
		}

		return;
	}
#endif /* KN_CUSTOM */

	case KN_TAG_AST: {
		struct kn_ast *ast = kn_value_as_ast(value);

		printf("Function(%s", ast->func->name);

		for (size_t i = 0; i < ast->func->arity; ++i) {
			printf(", ");
			kn_value_dump(ast->args[i]);
		}

		printf(")");
		return;
	}

	default:
		KN_UNREACHABLE();
	}
}

kn_value kn_value_run(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (KN_EXPECT(kn_tag(value), KN_TAG_AST)) {
	case KN_TAG_AST:
		return kn_ast_run(kn_value_as_ast(value));

	case KN_TAG_VARIABLE:
		return kn_variable_run(kn_value_as_variable(value));

	case KN_TAG_STRING:
	case KN_TAG_LIST:
		++*KN_REFCOUNT(value);
		// fallthrough

	case KN_TAG_NUMBER:
	case KN_TAG_CONSTANT:
		return value;

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->run != NULL) {
			return custom->vtable->run(custom->data);
		} else {
			return kn_value_clone(value);
		}
	}
#endif /* KN_CUSTOM */

	default:
		KN_UNREACHABLE();
	}
}

kn_value kn_value_clone(kn_value value) {
	assert(value != KN_UNDEFINED);

	if (KN_TAG_VARIABLE < kn_tag(value))
		++*KN_REFCOUNT(value);

	return value;
}

void kn_value_free(kn_value value) {
	assert(value != KN_UNDEFINED);

	if (kn_tag(value) <= KN_TAG_VARIABLE)
		return;

	if (KN_LIKELY(--*KN_REFCOUNT(value)))
		return;

	switch (KN_EXPECT(kn_tag(value), KN_TAG_STRING)) {

	case KN_TAG_STRING:
		kn_string_deallocate(kn_value_as_string(value));
		return;

	case KN_TAG_LIST:
		kn_list_deallocate(kn_value_as_list(value));
		return;

	case KN_TAG_AST:
		kn_ast_deallocate(kn_value_as_ast(value));
		return;

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM:
		kn_custom_deallocate(kn_value_as_custom(value));
		return;
#endif /* KN_CUSTOM */

	case KN_TAG_CONSTANT:
	case KN_TAG_NUMBER:
	case KN_TAG_VARIABLE:
	default:
		KN_UNREACHABLE();
	}
}
