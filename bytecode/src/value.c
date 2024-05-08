#include "value.h"
#include "string.h"
#include "list.h"
#include <math.h>

void kn_value_dealloc(kn_value value) {
	(void) value;
	// todo
}

void kn_value_dump(kn_value value, FILE *out) {
	static const char *constant_debugs[] = {
		[KN_TRUE]  = "true",
		[KN_FALSE] = "false",
		[KN_NULL]  = "null"
	};

	switch (kn_tag(value)) {
	case KN_VTAG_CONSTANT:
		kn_assert(value == KN_TRUE || value == KN_FALSE || value == KN_NULL);
		fputs(constant_debugs[value], out);
		return;

	case KN_VTAG_INTEGER:
		fprintf(out, "%" PRIdkn, kn_value_as_integer(value));
		return;

	case KN_VTAG_STRING:
		kn_string_dump(kn_value_as_string(value), out);
		return;

	case KN_VTAG_LIST:
		kn_list_dump(kn_value_as_list(value), out);
		return;
	case KN_VTAG_BLOCKREF:
		fprintf(out, "<block @%lu>", value >> 3);
		break;

#ifdef KN_CUSTOM
	case KN_VTAG_CUSTOM: {
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

kn_integer kn_value_to_integer(kn_value value) {
	kn_assert(value != KN_UNDEFINED);

	switch (KN_EXPECT(kn_tag(value), KN_VTAG_INTEGER)) {
	case KN_VTAG_CONSTANT:
		value >>= 1; // horray for micro-optimizations
		KN_FALLTHROUGH

	case KN_VTAG_INTEGER:
		return (kn_integer) value >> KN_SHIFT;

	case KN_VTAG_STRING:
		return kn_string_to_integer(kn_value_as_string(value));

	case KN_VTAG_LIST:
		return (kn_integer) kn_value_as_list(value)->length;

#ifdef KN_CUSTOM
	case KN_VTAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_integer != NULL)
			return custom->vtable->to_integer(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_VTAG_BLOCKREF:
		kn_bug("todo");

	default:
		KN_UNREACHABLE
	}
}

kn_boolean kn_value_to_boolean(kn_value value) {
	kn_assert(value != KN_UNDEFINED);

	switch (kn_tag(value)) {
#ifdef KN_CUSTOM
	case KN_VTAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_boolean != NULL)
			return custom->vtable->to_boolean(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_VTAG_BLOCKREF:
		kn_bug("todo");

	case KN_VTAG_CONSTANT:
	case KN_VTAG_INTEGER:
		return KN_NULL < value;

	case KN_VTAG_STRING:
		return kn_value_as_string(value)->length != 0;

	case KN_VTAG_LIST:
		return kn_value_as_list(value)->length != 0;

	default:
		KN_UNREACHABLE
	}
}



struct kn_string *kn_value_to_string(kn_value value) {
	// static, embedded strings so we don't have to allocate for known strings.
	static struct kn_string builtin_strings[KN_TRUE + 1] = {
		[KN_FALSE] = KN_STRING_NEW_EMBED("false"),
		[KN_INTERNAL_VALUE_NEW_INTEGER(0)]  = KN_STRING_NEW_EMBED("0"),
		[KN_NULL]  = KN_STRING_NEW_EMBED(""), // TODO: b/c its not `kn_string_empty`, will this err?
		[KN_INTERNAL_VALUE_NEW_INTEGER(1)]   = KN_STRING_NEW_EMBED("1"),
		[KN_TRUE]  = KN_STRING_NEW_EMBED("true"),
	};

	kn_assert(value != KN_UNDEFINED);

	if (KN_UNLIKELY(value <= KN_TRUE))
		return &builtin_strings[value];

	switch (KN_EXPECT(kn_tag(value), KN_VTAG_STRING)) {
	case KN_VTAG_STRING:
		return kn_value_as_string(value);

	case KN_VTAG_INTEGER:
		return kn_integer_to_string(kn_value_as_integer(value));

 	case KN_VTAG_LIST:
 		return kn_list_to_string(kn_value_as_list(value));

#ifdef KN_CUSTOM
	case KN_VTAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_string != NULL)
			return custom->vtable->to_string(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_VTAG_BLOCKREF:
		kn_bug("todo");

	case KN_VTAG_CONSTANT:
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
	case KN_VTAG_CONSTANT:
		return value == KN_TRUE ? &true_list : &kn_list_empty;

	case KN_VTAG_INTEGER:
		return kn_integer_to_list(kn_value_as_integer(value));

	case KN_VTAG_STRING:
		return kn_string_to_list(kn_value_as_string(value));

	case KN_VTAG_LIST:
		return kn_value_as_list(value);

#ifdef KN_CUSTOM
	case KN_VTAG_CUSTOM: {
		struct kn_custom *custom = kn_value_as_custom(value);

		if (custom->vtable->to_list != NULL)
			return custom->vtable->to_list(custom->data);
		KN_FALLTHROUGH
	}
#endif /* KN_CUSTOM */

	case KN_VTAG_BLOCKREF:
		kn_bug("todo");

	KN_DEFAULT_UNREACHABLE
	}
}

bool kn_value_equal(kn_value lhs, kn_value rhs) {
	kn_assert_ne(lhs, KN_UNDEFINED);
	kn_assert_ne(rhs, KN_UNDEFINED);

	if (lhs == rhs)
		return true;

	if (kn_tag(lhs) != kn_tag(rhs))
		return false;

	switch (kn_tag(lhs)) {
	case KN_VTAG_STRING:
		return kn_string_equal(kn_value_as_string(lhs), kn_value_as_string(rhs));

	case KN_VTAG_LIST:
		return kn_list_equal(kn_value_as_list(lhs), kn_value_as_list(rhs));

	default:
		return false;
	}
}

kn_value kn_value_add(kn_value lhs, kn_value rhs) {
	switch (kn_tag(lhs)) {
	case KN_VTAG_LIST:
		return kn_value_new_list(kn_list_concat(
			kn_value_as_list(lhs),
			kn_value_to_list(rhs)
		));

	case KN_VTAG_STRING:
		return kn_value_new_string(kn_string_concat(
			kn_value_as_string(lhs),
			kn_value_to_string(rhs)
		));

	case KN_VTAG_INTEGER:
		return kn_value_new_integer(kn_value_as_integer(lhs) + kn_value_to_integer(rhs));

	default:
		kn_die("can only add to strings, integers, and lists");
	}
}

#define kn_error(...) kn_die(__VA_ARGS__)

kn_value kn_value_sub(kn_value lhs, kn_value rhs) {
	if (!kn_value_is_integer(lhs))
		kn_die("can only subtract from integers");

	return kn_value_new_integer(kn_value_as_integer(lhs) - kn_value_to_integer(rhs));
}

kn_value kn_value_mul(kn_value lhs, kn_value rhs) {
	kn_integer amnt = kn_value_to_integer(rhs);

	if (kn_value_is_integer(lhs))
		return kn_value_new_integer(kn_value_as_integer(lhs) * amnt);

	if (amnt < 0 || amnt != (kn_integer) (size_t) amnt)
		kn_error("negative or too large an amount given");

	if (!kn_value_is_list(lhs) && !kn_value_is_string(lhs))
		kn_error("can only multiply integers, strings, and lists");

	// If lhs is a string, convert amnt to an integer and multiply.
	if (kn_value_is_string(lhs))
		return kn_value_new_string(kn_string_repeat(kn_value_as_string(lhs), amnt));

	return kn_value_new_list(kn_list_repeat(kn_value_as_list(lhs), amnt));
}

kn_value kn_value_div(kn_value lhs, kn_value rhs) {
	if (!kn_value_is_integer(lhs))
		kn_error("can only divide integers");

	kn_integer divisor = kn_value_to_integer(rhs);

	if (divisor == 0)
		kn_error("attempted to divide by zero");

	return kn_value_new_integer(kn_value_as_integer(lhs) / divisor);
}

kn_value kn_value_mod(kn_value lhs, kn_value rhs) {
	if (!kn_value_is_integer(lhs))
		kn_error("can only modulo integers");

	kn_integer base = kn_value_to_integer(rhs);

	if (base == 0)
		kn_error("attempted to modulo by zero");

	return kn_value_new_integer(kn_value_as_integer(lhs) % base);
}

kn_value kn_value_pow(kn_value lhs, kn_value rhs) {
	if (!kn_value_is_list(lhs) && !kn_value_is_integer(lhs))
		kn_error("can only exponentiate integers and lists");

	if (kn_value_is_integer(lhs)) {
		KN_CLANG_DIAG_PUSH
		KN_CLANG_DIAG_IGNORE("-Wbad-function-cast") // useless warning here.

		kn_integer power = (kn_integer) powl(
			(long double) kn_value_as_integer(lhs),
			(long double) kn_value_to_integer(rhs)
		);

		KN_CLANG_DIAG_POP

		return kn_value_new_integer(power);
	}

	struct kn_list *list = kn_value_as_list(lhs);
	struct kn_string *sep = kn_value_to_string(rhs);
	struct kn_string *joined = kn_list_join(list, sep);

	return kn_value_new_string(joined);
}

kn_integer kn_value_compare(kn_value lhs, kn_value rhs) {
	switch (kn_tag(lhs)) {
	case KN_VTAG_CONSTANT:
		return kn_value_as_boolean(lhs) - kn_value_to_boolean(rhs);

	case KN_VTAG_INTEGER:
		return kn_value_as_integer(lhs) - kn_value_to_integer(rhs);

	case KN_VTAG_STRING: {
		struct kn_string *rstring = kn_value_to_string(rhs);
		kn_value cmp = kn_string_compare(kn_value_as_string(lhs), rstring);
		return cmp;
	}

	case KN_VTAG_LIST: {
		struct kn_list *rlist = kn_value_to_list(rhs);
		kn_value cmp = kn_list_compare(kn_value_as_list(lhs), rlist);
		return cmp;
	}

	default:
		kn_error("can only compare boolean, integer, list, and string.");
	}
}
