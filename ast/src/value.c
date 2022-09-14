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

#define KN_REFCOUNT(x) ((size_t *) KN_UNMASK(x))

kn_number kn_value_to_number(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (KN_EXPECT(kn_tag(value), KN_TAG_NUMBER)) {
	case KN_TAG_NUMBER:
		return kn_value_as_number(value);

	case KN_TAG_CONSTANT:
		return value == KN_TRUE;

	case KN_TAG_STRING:
		return kn_string_to_number(kn_value_as_string(value));

	case KN_TAG_LIST:
		return kn_list_to_number(kn_value_as_list(value));

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_number != NULL)
			return custom->vtable->to_number(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_TAG_VARIABLE:
	case KN_TAG_AST:
		// simply execute the value and call this function again.
		value = kn_value_run(value);
		kn_number ret = kn_value_to_number(value);
		kn_value_free(value);
		return ret;
	}

	KN_UNREACHABLE();
}

kn_boolean kn_value_to_boolean(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (kn_tag(value)) {
	case KN_TAG_CONSTANT:
		return value == KN_TRUE;

	case KN_TAG_NUMBER:
		return kn_number_to_boolean(kn_value_to_number(value));

	case KN_TAG_STRING:
		return kn_string_to_boolean(kn_value_as_string(value));

	case KN_TAG_LIST:
		return kn_list_to_boolean(kn_value_to_list(value));

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_boolean != NULL)
			return custom->vtable->to_boolean(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_TAG_AST:
	case KN_TAG_VARIABLE:
		// simply execute the value and call this function again.
		value = kn_value_run(value);
		kn_boolean ret = kn_value_to_boolean(value);
		kn_value_free(value);
		return ret;
	}
}

struct kn_string *kn_value_to_string(kn_value value) {
	// static, embedded strings so we don't have to allocate for known strings.
	static struct kn_string builtin_strings[KN_TRUE + 1] = {
		[KN_FALSE] = KN_STRING_NEW_EMBED("false"),
		[KN_ZERO]  = KN_STRING_NEW_EMBED("0"),
		[KN_NULL]  = KN_STRING_NEW_EMBED("null"),
		[KN_ONE]   = KN_STRING_NEW_EMBED("1"),
		[KN_TRUE]  = KN_STRING_NEW_EMBED("true"),
	};

	assert(value != KN_UNDEFINED);

	if (KN_UNLIKELY(value <= KN_TRUE))
		return &builtin_strings[value];

	switch (KN_EXPECT(kn_tag(value), KN_TAG_STRING)) {
	case KN_TAG_STRING:
		return kn_string_clone(kn_value_as_string(value));

	case KN_TAG_NUMBER:
		return kn_number_to_string(kn_value_as_number(value));

 	case KN_TAG_LIST:
 		return kn_list_to_string(kn_value_as_list(value));

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_string != NULL)
			return custom->vtable->to_string(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_TAG_AST:
	case KN_TAG_VARIABLE: 
		// simply execute the value and call this function again.
		value = kn_value_run(value);
		struct kn_string *ret = kn_value_to_string(value);
		kn_value_free(value);
		return ret;

	case KN_TAG_CONSTANT:
		KN_UNREACHABLE();
	}

	KN_UNREACHABLE();
}


struct kn_list *kn_value_to_list(kn_value value) {
	static kn_value true_elements = KN_TRUE;
	static struct kn_list true_list = {
		.refcount = 1,
		.length = 1,
		.elements = &true_elements,
	};

	switch (kn_tag(value)) {
	case KN_TAG_CONSTANT:
		return value == KN_TRUE ? &true_list : &kn_list_empty;

	case KN_TAG_NUMBER:
		return kn_number_to_list(kn_value_as_number(value));

	case KN_TAG_STRING:
		return kn_string_to_list(kn_value_as_string(value));

	case KN_TAG_LIST:
		return kn_list_clone(kn_value_as_list(value));

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_list != NULL)
			return custom->vtable->to_list(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_TAG_AST:
	case KN_TAG_VARIABLE:
		// simply execute the value and call this function again.
		value = kn_value_run(value);
		struct kn_list *ret = kn_value_to_list(value);
		kn_value_free(value);
		return ret;
	}
}

bool kn_value_equal(kn_value lhs, kn_value rhs) {
	assert(lhs != KN_UNDEFINED);
	assert(rhs != KN_UNDEFINED);

	if (lhs == rhs)
		return true;

	if (kn_tag(lhs) != kn_tag(rhs))
		return false;

	switch (kn_tag(lhs)) {
	case KN_TAG_STRING:
		return kn_string_equal(kn_value_as_string(lhs), kn_value_as_string(rhs));

	case KN_TAG_LIST:
		return kn_list_equal(kn_value_as_list(lhs), kn_value_as_list(rhs));

	default:
		return false;
	}
}

kn_number kn_value_compare(kn_value lhs, kn_value rhs) {
	switch (kn_tag(lhs)) {
	case KN_TAG_NUMBER:
	case KN_TAG_CONSTANT:
		return lhs - rhs;
	case KN_TAG_STRING: {
		struct kn_string *rstring = kn_value_to_string(rhs);
		kn_value cmp = kn_string_compare(kn_value_as_string(lhs), rstring);
		kn_string_free(rstring);
		return cmp;
	}
	case KN_TAG_LIST: {
		struct kn_list *rlist = kn_value_to_list(rhs);
		kn_value cmp = kn_list_compare(kn_value_as_list(lhs), rlist);
		kn_list_free(rlist);
		return cmp;
	}
	default:
		kn_error("can only compare boolean, number, list, and string.");
	}
}


void kn_value_dump(kn_value value, FILE *out) {
	switch (kn_tag(value)) {
	case KN_TAG_CONSTANT:
		switch (value) {
		case KN_TRUE:  fputs("Boolean(true)", out); return;
		case KN_FALSE: fputs("Boolean(false)", out); return;
		case KN_NULL:  fputs("Null()", out); return;
#ifndef NDEBUG // we dump undefined only for debugging.
		case KN_UNDEFINED: fputs("<KN_UNDEFINED>", out); return;
#endif /* !NDEBUG */
		default: KN_UNREACHABLE();
		}

	case KN_TAG_NUMBER:
		kn_number_dump(kn_value_as_number(value), out);
		return;

	case KN_TAG_STRING:
		kn_string_dump(kn_value_as_string(value), out);
		return;

	case KN_TAG_LIST:
		kn_list_dump(kn_value_as_list(value), out);
		return;

	case KN_TAG_VARIABLE:
		kn_variable_dump(kn_value_as_variable(value), out);
		return;

	case KN_TAG_AST:
		kn_ast_dump(kn_value_as_ast(value), out);
		return;


#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->dump != NULL) {
			custom->vtable->dump(custom->data);
		} else {
			fprintf(out,
				"Custom(%p, %p)", (void *) custom->data, (void *) custom->vtable
			);
		}

		return;
	}
#endif /* KN_CUSTOM */
	}
}

kn_value kn_value_run(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (KN_EXPECT(kn_tag(value), KN_TAG_AST)) {
	case KN_TAG_AST:
		return kn_ast_run(kn_value_as_ast(value));

	case KN_TAG_VARIABLE:
		return kn_variable_run(kn_value_as_variable(value));

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->run != NULL)
			return custom->vtable->run(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_TAG_STRING:
	case KN_TAG_LIST:
		++*KN_REFCOUNT(value);
		KN_FALLTHROUGH

	case KN_TAG_NUMBER:
	case KN_TAG_CONSTANT:
		return value;

	default:
		KN_UNREACHABLE();
	}
}

static bool is_allocated(kn_value value) {
	return KN_TAG_VARIABLE < kn_tag(value);
}

kn_value kn_value_clone(kn_value value) {
	assert(value != KN_UNDEFINED);

	if (is_allocated(value))
		++*KN_REFCOUNT(value);

	return value;
}

void kn_value_free(kn_value value) {
	assert(value != KN_UNDEFINED);

	if (!is_allocated(value))
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
		KN_UNREACHABLE();
	}
}
