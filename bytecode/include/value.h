#ifndef KN_VALUE_H
#define KN_VALUE_H

#include "types.h"
#include "debug.h"
// #include "shared.h"
// #include "container.h"
// #include <stdio.h>

#define KN_VALUE_ALIGNMENT 8
#define KN_VALUE_ALIGN _Alignas(KN_VALUE_ALIGNMENT)
KN_STATIC_ASSERTM(_Alignof(kn_value) >= 8, "tags won't work!");

/**
 * The layout of `kn_value`:
 * 0...00000 - FALSE
 * 0...01000 - NULL
 * 0...10000 - TRUE
 * 0...11000 - KN_UNDEFINED
 * X...XX001 - 61-bit signed integer
 * X...XX010 - variable
 * X...XX011 - string
 * X...XX100 - function
 * X...XX101 - list
 * X...XX110 - custom (only with `KN_CUSTOM`)
 * note all pointers are 8-bit-aligned.
 **/
enum kn_value_tag {
	KN_VTAG_CONSTANT = 0,
	KN_VTAG_INTEGER  = 1,
//	KN_VTAG_VARIABLE = 2,
	KN_VTAG_BLOCKREF = 3,
	KN_VTAG_STRING   = 4,
	KN_VTAG_LIST     = 5
#ifdef KN_CUSTOM
	, KN_VTAG_CUSTOM = 6 // if a new enum is added, update the kn_assert below.
# define KN_VTAG_LAST KN_VTAG_CUSTOM
#else
# define KN_VTAG_LAST KN_VTAG_LIST
#endif /* KN_CUSTOM */
};

#define KN_SHIFT 3
#define KN_VTAG_MASK ((kn_value) ((1 << KN_SHIFT) - 1))

KN_STATIC_ASSERTM((KN_VTAG_MASK + 1) == KN_VALUE_ALIGNMENT, "need to change alignment");
KN_STATIC_ASSERTM((KN_VTAG_MASK & KN_VTAG_LIST) == KN_VTAG_LIST, "mask doesnt cover all values");

/**
 * The false value within Knight.
 **/
#define KN_FALSE ((0 << KN_SHIFT) | KN_VTAG_CONSTANT)

/**
 * The null value within Knight.
 **/
#define KN_NULL ((1 << KN_SHIFT) | KN_VTAG_CONSTANT)

/**
 * The true value within Knight.
 **/
#define KN_TRUE ((2 << KN_SHIFT) | KN_VTAG_CONSTANT)

/**
 * An undefined value, used to indicate "no value."
 *
 * This is used in a few places, such as the default value for variables, and
 * what's returned from `kn_parse` if no values could e parsed. This value is
 * invalid to pass to any function expecting a valid `kn_value`.
 **/
#define KN_UNDEFINED  ((3 << KN_SHIFT) | KN_VTAG_CONSTANT)

#define KN_UNMASK(x) ((x) & ~KN_VTAG_MASK)

static inline enum kn_value_tag kn_tag(kn_value value) {
	return value & KN_VTAG_MASK;
}

/**************************************************************************************************
 *                                                                                                *
 *                                       Value Constructors                                       *
 *                                                                                                *
 **************************************************************************************************/

#define KN_INTERNAL_VALUE_NEW_INTEGER(val) ((((kn_value) val) << KN_SHIFT) | KN_VTAG_INTEGER) // used in value.c
/**
 * Creates a new integer value.
 *
 * Note that `integer` has to be a valid `kn_integer`---see its typedef for more details on what
 * this entails.
 **/
static inline kn_value kn_value_new_integer(kn_integer integer) {
	kn_value shifted = (kn_value) integer << KN_SHIFT;
	kn_assert_eq(integer, (kn_integer) shifted >> KN_SHIFT);
	kn_assert_eq(shifted | KN_VTAG_INTEGER, KN_INTERNAL_VALUE_NEW_INTEGER(integer));

	return shifted | KN_VTAG_INTEGER;
}

/**
 * Creates a new boolean value.
 *
 * If you know the value of `boolean` ahead of time, you should simply use `KN_TRUE` or `KN_FALSE`.
 **/
static inline kn_value kn_value_new_boolean(kn_boolean boolean) {
	return (kn_value) boolean << 4; // micro-optimizations hooray!
}

/**
 * Creates a new string value.
 *
 * This passes ownership of the string to this function, and any use of the passed pointer is
 * invalid after this function returns.
 **/
static inline kn_value kn_value_new_string(struct kn_string *KN_NONNULL string) {
	kn_assert_nonnull(string);

	kn_value string_value = (kn_value) string;
	kn_assert_eq(kn_tag(string_value), 0); // indicates a misaligned pointer

	return string_value | KN_VTAG_STRING;
}

/**
 * Creates a new list value.
 *
 * This passes ownership of the list to this function, and any use of the
 * passed pointer is invalid after this function returns.
 **/
static inline kn_value kn_value_new_list(struct kn_list *KN_NONNULL list) {
	kn_assert_nonnull(list);

	kn_value list_value = (kn_value) list;
	kn_assert_eq(kn_tag(list_value), 0); // indicates a misaligned pointer

	return list_value | KN_VTAG_LIST;
}

#if 0
/**
 * Creates a new variable value.
 **/
static inline kn_value kn_value_new_variable(struct kn_variable *variable) {
	// a nonzero tag indicates a misaligned pointer
	kn_assert(kn_tag((kn_value) variable) == 0);
	kn_assert(variable != NULL);

	return ((kn_value) variable) | KN_VTAG_VARIABLE;
}

/**
 * Creates a new ast value.
 *
 * This passes ownership of the ast to this function, and any use of the
 * passed pointer is invalid after this function returns.
 **/
static inline kn_value kn_value_new_ast(struct kn_ast *ast) {
	// a nonzero tag indicates a misaligned pointer
	kn_assert(kn_tag((kn_value) ast) == 0);
	kn_assert(ast != NULL);

	return ((kn_value) ast) | KN_VTAG_AST;
}
#endif

#ifdef KN_CUSTOM
/**
 * Creates a new custom value.
 *
 * Ownership of the `custom` is passed to this function.
 **/
static inline kn_value kn_value_new_custom(struct kn_custom *KN_NONNULL custom) {

	// a nonzero tag indicates a misaligned pointer
	kn_assert(kn_tag((kn_value) custom) == 0);
	kn_assert(custom != NULL);
	kn_assert(custom->vtable != NULL);

	return ((kn_value) custom) | KN_VTAG_CUSTOM;
}
#endif /* KN_CUSTOM */

/**************************************************************************************************
 *                                                                                                *
 *                                        Checking Values                                         *
 *                                                                                                *
 **************************************************************************************************/

/**
 * Checks to see if `value` is a `kn_integer`.
 **/
static inline bool kn_value_is_integer(kn_value value) {
	return kn_tag(value) == KN_VTAG_INTEGER;
}

/**
 * Checks to see if `value` is a `KN_TRUE` or `KN_FALSE`.
 **/
static inline bool kn_value_is_boolean(kn_value value) {
	return value == KN_FALSE || value == KN_TRUE;
}

/**
 * Note there's no `kn_value_is_null`, as you can simply do `value == KN_NULL`.
 **/

/**
 * Checks to see if `value` is a `kn_string`.
 **/
static inline bool kn_value_is_string(kn_value value) {
	return kn_tag(value) == KN_VTAG_STRING;
}

/**
 * Checks to see if `value` is a `kn_list`.
 **/
static inline bool kn_value_is_list(kn_value value) {
	return kn_tag(value) == KN_VTAG_LIST;
}

#if 0
/**
 * Checks to see if `value` is a `kn_variable`.
 **/
static inline bool kn_value_is_variable(kn_value value) {
	return kn_tag(value) == KN_VTAG_VARIABLE;
}

/**
 * Checks to see if `value` is a `kn_ast`.
 **/
static inline bool kn_value_is_ast(kn_value value) {
	return kn_tag(value) == KN_VTAG_AST;
}
#endif

#ifdef KN_CUSTOM
/**
 * Checks to see if `value` is a `kn_custom`.
 **/
static inline bool kn_value_is_custom(kn_value value) {
	return kn_tag(value) == KN_VTAG_CUSTOM;
}
#endif /* KN_CUSTOM */


/**************************************************************************************************
 *                                                                                                *
 *                                       Value Downcasting                                        *
 *                                                                                                *
 **************************************************************************************************/

/**
 * Retrieves the `kn_integer` associated with `value`.
 *
 * This should only be called on integer values.
 **/
static inline kn_integer kn_value_as_integer(kn_value value) {
	kn_assert(kn_value_is_integer(value));
	return ((kn_integer) value) >> KN_SHIFT;
}

/**
 * Retrieves the `kn_boolean` associated with `value`.
 *
 * This should only be called on boolean values.
 **/
static inline kn_boolean kn_value_as_boolean(kn_value value) {
	kn_assert(kn_value_is_boolean(value));
	return value != KN_FALSE;
}

/**
 * Retrieves the `kn_string` associated with `value`.
 *
 * This should only be called on string values.
 **/
static inline struct kn_string *KN_NONNULL kn_value_as_string(kn_value value) {
	kn_assert(kn_value_is_string(value));
	return (struct kn_string *) KN_UNMASK(value);
}

/**
 * Retrieves the `kn_list` associated with `value`.
 *
 * This should only be called on list values.
 **/
static inline struct kn_list *KN_NONNULL kn_value_as_list(kn_value value) {
	kn_assert(kn_value_is_list(value));
	return (struct kn_list *) KN_UNMASK(value);
}

#if 0
/**
 * Retrieves the `kn_variable` associated with `value`.
 *
 * This should only be called on variable values.
 **/
static inline struct kn_variable *kn_value_as_variable(kn_value value) {
	kn_assert(kn_value_is_variable(value));
	return (struct kn_variable *) KN_UNMASK(value);
}

/**
 * Retrieves the `kn_ast` associated with `value`.
 *
 * This should only be called on ast values.
 **/
static inline struct kn_ast *kn_value_as_ast(kn_value value) {
	kn_assert(kn_value_is_ast(value));
	return (struct kn_ast *) KN_UNMASK(value);
}
#endif

#ifdef KN_CUSTOM
/**
 * Retrieves the `kn_custom` associated with `value`.
 *
 * This should only be called on custom values.
 **/
static inline struct kn_custom *KN_NONNULL kn_value_as_custom(kn_value value) {
	kn_assert(kn_value_iscustom(value));
	return (struct kn_custom *) KN_UNMASK(value);
}
#endif /* KN_CUSTOM */

/**************************************************************************************************
 *                                                                                                *
 *                                       Value Conversions                                        *
 *                                                                                                *
 **************************************************************************************************/

/**
 * Converts the `value` to a `kn_integer`, coercing it if need be.
 **/
kn_integer kn_value_to_integer(kn_value value);

/**
 * Converts the `value` to a `kn_boolean`, coercing it if need be.
 **/
kn_boolean kn_value_to_boolean(kn_value value);

/**
 * Converts the `value` to a `kn_string`, coercing it if need be.
 *
 * Note that the caller must free the returned string via `kn_string_free`.
 **/
struct kn_string *KN_NONNULL kn_value_to_string(kn_value value);

/**
 * Converts the `value` to a `kn_list`, coercing it if need be.
 *
 * Note that the caller must free the returned list via `kn_list_free`.
 **/
struct kn_list *KN_NONNULL kn_value_to_list(kn_value value);

/**************************************************************************************************
 *                                                                                                *
 *                                              Misc                                              *
 *                                                                                                *
 **************************************************************************************************/

/**
 * Dumps the debugging representation of `value` to stdout, without a trailing
 * newline.
 **/
void kn_value_dump(kn_value value, FILE *KN_NONNULL out);

void kn_value_mark(kn_value value);
void kn_value_dealloc(kn_value value);
bool kn_value_equal(kn_value lhs, kn_value rhs);
kn_integer kn_value_compare(kn_value lhs, kn_value rhs);

kn_value kn_value_add(kn_value lhs, kn_value rhs);
kn_value kn_value_sub(kn_value lhs, kn_value rhs);
kn_value kn_value_mul(kn_value lhs, kn_value rhs);
kn_value kn_value_div(kn_value lhs, kn_value rhs);
kn_value kn_value_mod(kn_value lhs, kn_value rhs);
kn_value kn_value_pow(kn_value lhs, kn_value rhs);

kn_value kn_value_get(kn_value val, kn_value start, kn_value len);
kn_value kn_value_set(kn_value val, kn_value start, kn_value len, kn_value repl);

#endif /* !KN_VALUE_H */
