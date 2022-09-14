#ifndef KN_VALUE_H
#define KN_VALUE_H

#include "decls.h"
#include <assert.h>
#include <stdio.h>


/*
 * The layout of `kn_value`:
 * 0...00000 - FALSE
 * 0...01000 - NULL
 * 0...10000 - TRUE
 * 0...11000 - undefined.
 * X...XX001 - 61-bit signed integer
 * X...XX010 - variable
 * X...XX011 - string
 * X...XX100 - function
 * X...XX101 - list
 * X...XX110 - custom (only with `KN_CUSTOM`)
 * note all pointers are 8-bit-aligned.
 */
enum kn_value_tag {
   KN_TAG_CONSTANT = 0,
   KN_TAG_NUMBER = 1,
   KN_TAG_VARIABLE = 2,
   KN_TAG_STRING = 3,
   KN_TAG_AST = 4,
   KN_TAG_LIST = 5,
#ifdef KN_CUSTOM
   KN_TAG_CUSTOM = 6
#endif /* KN_CUSTOM */
};


#define KN_SHIFT 3
#define KN_TAG_MASK ((1 << KN_SHIFT) - 1)

/*
 * The false value within Knight.
 */
#define KN_FALSE ((0 << KN_SHIFT) | KN_TAG_CONSTANT)

/*
 * The null value within Knight.
 */
#define KN_NULL ((1 << KN_SHIFT) | KN_TAG_CONSTANT)

/*
 * The true value within Knight.
 */
#define KN_TRUE ((2 << KN_SHIFT) | KN_TAG_CONSTANT)

/*
 * An undefined value, used to indicate "no value."
 *
 * This is used in a few places, such as the default value for variables, and
 * what's returned from `kn_parse` if no values could e parsed. This value is
 * invalid to pass to any function expecting a valid `kn_value`.
 */
#define KN_UNDEFINED  ((3 << KN_SHIFT) | KN_TAG_CONSTANT)

#define KN_ONE ((((kn_number) 1) << KN_SHIFT) | KN_TAG_NUMBER)
#define KN_ZERO ((((kn_number) 0) << KN_SHIFT) | KN_TAG_NUMBER)

static inline enum kn_value_tag kn_tag(kn_value value) {
   return value & KN_TAG_MASK;
}

#ifdef KN_CUSTOM
#define KN_VALUE_NEW_MAYBE_CUSTOM(x) ,struct kn_custom *: kn_value_new_custom
#else
#define KN_VALUE_NEW_MAYBE_CUSTOM(x)
#endif

#define kn_value_new(x) (_Generic(x,            \
   kn_number: kn_value_new_number,              \
   kn_boolean: kn_value_new_boolean,            \
   struct kn_string *: kn_value_new_string,     \
   struct kn_list *: kn_value_new_list,         \
   struct kn_variable *: kn_value_new_variable, \
   struct kn_ast *: kn_value_new_ast            \
   KN_VALUE_NEW_MAYBE_CUSTOM(x)                 \
   )(x))

#define KN_UNMASK(x) ((x) & ~KN_TAG_MASK)

/*
 * Creates a new number value.
 *
 * Note that `number` has to be a valid `kn_number`---see its typedef for more
 * details on what this entails.
 */
static inline kn_value kn_value_new_number(kn_number number) {
   assert(number == (((kn_number) ((kn_value) number << KN_SHIFT)) >> KN_SHIFT));

   return (((kn_value) number) << KN_SHIFT) | KN_TAG_NUMBER;
}

/*
 * Creates a new boolean value.
 *
 * If you know the value of `boolean` ahead of time, you should simply use
 * `KN_TRUE` or `KN_FALSE`.
 */
static inline kn_value kn_value_new_boolean(kn_boolean boolean) {
   return ((kn_value) boolean) << 4; // micro-optimizations hooray!
}

/*
 * Creates a new string value.
 *
 * This passes ownership of the string to this function, and any use of the
 * passed pointer is invalid after this function returns.
 */
static inline kn_value kn_value_new_string(struct kn_string *string) {
   // a nonzero tag indicates a misaligned pointer
   assert(KN_TAG((kn_value) string) == 0);
   assert(string != NULL);

   return ((kn_value) string) | KN_TAG_STRING;
}

/*
 * Creates a new list value.
 *
 * This passes ownership of the list to this function, and any use of the
 * passed pointer is invalid after this function returns.
 */
static inline kn_value kn_value_new_list(struct kn_list *list) {
   // a nonzero tag indicates a misaligned pointer
   assert(KN_TAG((kn_value) list) == 0);
   assert(list != NULL);

   return ((kn_value) list) | KN_TAG_LIST;
}

/*
 * Creates a new variable value.
 */
static inline kn_value kn_value_new_variable(struct kn_variable *variable) {
   // a nonzero tag indicates a misaligned pointer
   assert(KN_TAG((kn_value) variable) == 0);
   assert(variable != NULL);

   return ((kn_value) variable) | KN_TAG_VARIABLE;
}

/*
 * Creates a new ast value.
 *
 * This passes ownership of the ast to this function, and any use of the
 * passed pointer is invalid after this function returns.
 */
static inline kn_value kn_value_new_ast(struct kn_ast *ast) {
   // a nonzero tag indicates a misaligned pointer
   assert(KN_TAG((kn_value) ast) == 0);
   assert(ast != NULL);

   return ((kn_value) ast) | KN_TAG_AST;
}

#ifdef KN_CUSTOM
/*
 * Creates a new custom value.
 *
 * Ownership of the `custom` is passed to this function.
 */
static inline kn_value kn_value_new_custom(struct kn_custom *custom) {
   // a nonzero tag indicates a misaligned pointer
   assert(KN_TAG((kn_value) custom) == 0);
   assert(custom != NULL);
   assert(custom->vtable != NULL);

   return ((kn_value) custom) | KN_TAG_CUSTOM;
}
#endif /* KN_CUSTOM */

/*
 * Checks to see if `value` is a `kn_number`.
 */
static inline bool kn_value_is_number(kn_value value) {
   return kn_tag(value) == KN_TAG_NUMBER;
}

/*
 * Checks to see if `value` is a `KN_TRUE` or `KN_FALSE`.
 */
static inline bool kn_value_is_boolean(kn_value value) {
   return value == KN_FALSE || value == KN_TRUE;
}

/*
 * Note there's no `kn_value_is_null`, as you can simply do `value == KN_NULL`.
 */

/*
 * Checks to see if `value` is a `kn_string`.
 */
static inline bool kn_value_is_string(kn_value value) {
   return kn_tag(value) == KN_TAG_STRING;
}

/*
 * Checks to see if `value` is a `kn_list`.
 */
static inline bool kn_value_is_list(kn_value value) {
   return kn_tag(value) == KN_TAG_LIST;
}

/*
 * Checks to see if `value` is a `kn_variable`.
 */
static inline bool kn_value_is_variable(kn_value value) {
   return kn_tag(value) == KN_TAG_VARIABLE;
}

/*
 * Checks to see if `value` is a `kn_ast`.
 */
static inline bool kn_value_is_ast(kn_value value) {
   return kn_tag(value) == KN_TAG_AST;
}

#ifdef KN_CUSTOM
/*
 * Checks to see if `value` is a `kn_custom`.
 */
static inline bool kn_value_is_custom(kn_value value) {
   return kn_tag(value) == KN_TAG_CUSTOM;
}
#endif /* KN_CUSTOM */

/*
 * Retrieves the `kn_number` associated with `value`.
 *
 * This should only be called on number values.
 */
static inline kn_number kn_value_as_number(kn_value value) {
   assert(kn_value_is_number(value));
   return ((kn_number) value) >> KN_SHIFT;
}

/*
 * Retrieves the `kn_boolean` associated with `value`.
 *
 * This should only be called on boolean values.
 */
static inline kn_boolean kn_value_as_boolean(kn_value value) {
   assert(kn_value_is_boolean(value));
   return value != KN_FALSE;
}

/*
 * Retrieves the `kn_string` associated with `value`.
 *
 * This should only be called on string values.
 */
static inline struct kn_string *kn_value_as_string(kn_value value) {
   assert(kn_value_is_string(value));
   return (struct kn_string *) KN_UNMASK(value);
}

/*
 * Retrieves the `kn_list` associated with `value`.
 *
 * This should only be called on list values.
 */
static inline struct kn_list *kn_value_as_list(kn_value value) {
   assert(kn_value_is_list(value));
   return (struct kn_list *) KN_UNMASK(value);
}

/*
 * Retrieves the `kn_variable` associated with `value`.
 *
 * This should only be called on variable values.
 */
static inline struct kn_variable *kn_value_as_variable(kn_value value) {
   assert(kn_value_is_variable(value));
   return (struct kn_variable *) KN_UNMASK(value);
}

/*
 * Retrieves the `kn_ast` associated with `value`.
 *
 * This should only be called on ast values.
 */
static inline struct kn_ast *kn_value_as_ast(kn_value value) {
   assert(kn_value_is_ast(value));
   return (struct kn_ast *) KN_UNMASK(value);
}

#ifdef KN_CUSTOM
/*
 * Retrieves the `kn_custom` associated with `value`.
 *
 * This should only be called on custom values.
 */
static inline struct kn_custom *kn_value_as_custom(kn_value value) {
   assert(kn_value_iscustom(value));
   return (struct kncustom *) KN_UNMASK(value);
}
#endif /* KN_CUSTOM */

/*
 * Converts the `value` to a `kn_number`, coercing it if need be.
 */
kn_number kn_value_to_number(kn_value value);

/*
 * Converts the `value` to a `kn_boolean`, coercing it if need be.
 */
kn_boolean kn_value_to_boolean(kn_value value);

/*
 * Converts the `value` to a `kn_string`, coercing it if need be.
 *
 * Note that the caller must free the returned string via `kn_string_free`.
 */
struct kn_string *kn_value_to_string(kn_value value);

/*
 * Converts the `value` to a `kn_list`, coercing it if need be.
 *
 * Note that the caller must free the returned list via `kn_list_free`.
 */
struct kn_list *kn_value_to_list(kn_value value);

/*
 * Dumps the debugging representation of `value` to stdout, without a trailing
 * newline.
 */
void kn_value_dump(kn_value value, FILE *out);

/*
 * Executes the given value.
 *
 * The returned value must be passed to `kn_value_free` to prevent memory leaks.
 */
kn_value kn_value_run(kn_value value);

/*
 * Returns a copy of `value`.
 *
 * Both `value` and the returned value must be passed independently to
 * `kn_value_free` to ensure that all resources are cleaned up after use.
 */
kn_value kn_value_clone(kn_value value);

/*
 * Frees all resources associated with `value`.
 *
 * Note that for the refcounted types (ie `kn_string` and `kn_ast`), this
 * will only actually free the resources when the refcount hits zero.
 */
void kn_value_free(kn_value value);

bool kn_value_equal(kn_value lhs, kn_value rhs);
kn_number kn_value_compare(kn_value lhs, kn_value rhs);

#endif /* !KN_VALUE_H */
