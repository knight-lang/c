#include "env.h"    /* kn_variable, kn_variable_run */
#include "ast.h"    /* kn_ast, kn_ast_dealloc, kn_ast_clone, kn_ast_run */
#include "value.h"  /* prototypes, bool, uint64_t, int64_t, kn_value, kn_integer,
                       kn_boolean, KN_UNDEFINED, KN_NULL, KN_TRUE, KN_FALSE */
#include "string.h" /* kn_string, kn_string_clone, kn_string_dealloc,
                       kn_string_deref, kn_string_length, KN_STRING_FL_STATIC,
                       KN_STRING_NEW_EMBED */
#include "custom.h" /* kn_custom, kn_custom_free, kn_custom_clone */
#include "integer.h" /* kn_integer_to_string */
#include "list.h"

#include <inttypes.h> /* PRId64 */
#include <stdlib.h>   /* free, NULL */
#include <assert.h>   /* assert */
#include <stdio.h>    /* printf */
#include <ctype.h>    /* isspace */

kn_integer kn_value_to_integer(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (KN_EXPECT(kn_tag(value), KN_TAG_INTEGER)) {
	case KN_TAG_CONSTANT:
		value >>= 1; // horray for micro-optimizations
		KN_FALLTHROUGH

	case KN_TAG_INTEGER:
		return (kn_integer) value >> KN_SHIFT;

	case KN_TAG_STRING:
		return kn_string_to_integer(kn_value_as_string(value));

	case KN_TAG_LIST:
		return (kn_integer) kn_length(kn_value_as_list(value));

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_integer != NULL)
			return custom->vtable->to_integer(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_TAG_VARIABLE:
	case KN_TAG_AST:
		// simply execute the value and call this function again.
		value = kn_value_run(value);
		kn_integer ret = kn_value_to_integer(value);
		kn_value_free(value);
		return ret;
	}

	KN_UNREACHABLE
}

kn_boolean kn_value_to_boolean(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (kn_tag(value)) {
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
		if (kn_tag(value) <= KN_TAG_INTEGER) {
	case KN_TAG_CONSTANT:
	case KN_TAG_INTEGER:
			return KN_NULL < value;
		}

		kn_boolean ret = kn_container_length(value) != 0;

#ifdef KN_USE_REFCOUNT
		if (!--*kn_container_refcount(value))
			kn_value_dealloc(value);
#endif /* KN_USE_REFCOUNT */

		return ret;

	case KN_TAG_STRING:
	case KN_TAG_LIST:
		return kn_container_length(value) != 0;

	KN_DEFAULT_UNREACHABLE
	}
}

struct kn_string *kn_value_to_string(kn_value value) {
	// static, embedded strings so we don't have to allocate for known strings.
	static struct kn_string builtin_strings[KN_TRUE + 1] = {
		[KN_FALSE] = KN_STRING_NEW_EMBED("false"),
		[KN_ZERO]  = KN_STRING_NEW_EMBED("0"),
		[KN_NULL]  = KN_STRING_NEW_EMBED(""), // TODO: b/c its not `kn_string_empty`, will this err?
		[KN_ONE]   = KN_STRING_NEW_EMBED("1"),
		[KN_TRUE]  = KN_STRING_NEW_EMBED("true"),
	};

	assert(value != KN_UNDEFINED);

	if (KN_UNLIKELY(value <= KN_TRUE))
		return &builtin_strings[value];

	switch (KN_EXPECT(kn_tag(value), KN_TAG_STRING)) {
	case KN_TAG_STRING:
		return kn_string_clone(kn_value_as_string(value));

	case KN_TAG_INTEGER:
		return kn_integer_to_string(kn_value_as_integer(value));

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
		KN_UNREACHABLE
	}

	KN_UNREACHABLE
}


struct kn_list *kn_value_to_list(kn_value value) {
	static struct kn_list true_list = {
#ifdef KN_USE_REFCOUNT
		.refcount = 1,
#endif /* KN_USE_REFCOUNT */
		.flags = KN_LIST_FL_STATIC | KN_LIST_FL_EMBED,
		.length = 1,
		.embed = { KN_TRUE },
	};

	switch (kn_tag(value)) {
	case KN_TAG_CONSTANT:
		return kn_list_clone(value == KN_TRUE ? &true_list : &kn_list_empty); // todo: is this clone correct?

	case KN_TAG_INTEGER:
		return kn_integer_to_list(kn_value_as_integer(value));

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

	KN_DEFAULT_UNREACHABLE
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

kn_integer kn_value_compare(kn_value lhs, kn_value rhs) {
	switch (kn_tag(lhs)) {
	case KN_TAG_CONSTANT:
		return kn_value_as_boolean(lhs) - kn_value_to_boolean(rhs);

	case KN_TAG_INTEGER:
		return kn_value_as_integer(lhs) - kn_value_to_integer(rhs);

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
		kn_error("can only compare boolean, integer, list, and string.");
	}
}


void kn_value_dump(kn_value value, FILE *out) {
	switch (kn_tag(value)) {
	case KN_TAG_CONSTANT:
		switch (value) {
		case KN_TRUE:
			fputs("true", out);
			return;

		case KN_FALSE:
			fputs("false", out);
			return;

		case KN_NULL:
			fputs("null", out);
			return;

		default:
			KN_UNREACHABLE
		}

	case KN_TAG_INTEGER:
		fprintf(out, "%" PRIdkn, kn_value_as_integer(value));
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
	KN_DEFAULT_UNREACHABLE
	}
}

kn_value kn_value_run(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (kn_tag(value)) {
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
#ifdef KN_USE_REFCOUNT
		++*kn_container_refcount(value);
#endif /* KN_USE_REFCOUNT */
		; KN_FALLTHROUGH

	case KN_TAG_INTEGER:
	case KN_TAG_CONSTANT:
		return value;

	KN_DEFAULT_UNREACHABLE
	}
}


void
#if KN_HAS_ATTRIBUTE(noinline)
KN_ATTRIBUTE(noinline)
#endif
kn_value_dealloc(kn_value value) {
	assert(value != KN_UNDEFINED);

	switch (kn_tag(value)) {
	case KN_TAG_STRING:
		kn_string_dealloc(kn_value_as_string(value));
		return;

	case KN_TAG_LIST:
		kn_list_dealloc(kn_value_as_list(value));
		return;

	case KN_TAG_AST:
		kn_ast_dealloc(kn_value_as_ast(value));
		return;

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM:
		kn_custom_dealloc(kn_value_as_custom(value));
		return;
#endif /* KN_CUSTOM */

	case KN_TAG_CONSTANT:
	case KN_TAG_INTEGER:
	case KN_TAG_VARIABLE:
		KN_UNREACHABLE

	KN_DEFAULT_UNREACHABLE
	}
}

#ifdef KN_USE_GC
void kn_value_mark(kn_value value) {
	switch (kn_tag(value)) {
	case KN_TAG_CONSTANT:
	case KN_TAG_INTEGER:
		return;

	case KN_TAG_STRING:
		return;

	case KN_TAG_LIST:
		return;

	case KN_TAG_VARIABLE:
		return;

	case KN_TAG_AST:
		kn_ast_mark(kn_value_as_ast(value));
		return;

#ifdef KN_CUSTOM
	case KN_TAG_CUSTOM:
		// kn_custom_dealloc(kn_value_as_custom(value));
		return;
#endif /* KN_CUSTOM */
	}
}
#endif
