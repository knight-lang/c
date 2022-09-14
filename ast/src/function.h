#ifndef KN_FUNCTION_H
#define KN_FUNCTION_H

#include "value.h" /* kn_value */

/*
 * The maximum argc for functions. Used for optimizations in some places.
 */
#ifndef KN_MAX_ARGC
# define KN_MAX_ARGC 4
#endif /* !KN_MAX_ARGC */

/*
 * This struct is used to keep track of all data relevant for a Knight function.
 */
struct kn_function {
	/*
	 * A pointer to the function. It will take exactly `arity` arguments.
	 */
	kn_value (*func)(const kn_value *args);

	/*
	 * The number of arguments that `func` should expect.
	 */
	size_t arity;

	/*
	 * The name of the function; used when debugging.
	 */
	const char *name;
};

/*
 * Initializes all relevant data for functions.
 *
 * This should be called before any `kn_function.func` methods are called.
 */
void kn_function_startup(void);

/*
 * Declares a function with the given function name, arity, and char name.
 */
#define KN_DECLARE_FUNCTION(func_, arity_, name_)         \
   static kn_value func_##_function(const kn_value *);    \
   const struct kn_function func_ = {                     \
      .func = func_##_function,                           \
      .arity = arity_,                                    \
      .name = name_                                       \
   };                                                     \
   static kn_value func_##_function(const kn_value *args)

/******************************************************************************
 * The following are all of the different types of functions within Knight.   *
 * For details on what each function does, see the specs---these all conform. *
 ******************************************************************************/

/**
 * 4.1 Arity 0
 *
 * Note that the `TRUE`, `FALSE`, `NULL`, and `@` aren't functions, but rather literals.
 **/

extern const struct kn_function kn_fn_prompt;
extern const struct kn_function kn_fn_random;

/**
 * 4.2 Arity 1
 *
 * Note that the 4.2.1 (`:`) is treated as whitespace, and as such has no
 * associated function.
 **/

extern const struct kn_function kn_fn_noop;
extern const struct kn_function kn_fn_block;
extern const struct kn_function kn_fn_call;
extern const struct kn_function kn_fn_quit;
extern const struct kn_function kn_fn_not;
extern const struct kn_function kn_fn_length;
extern const struct kn_function kn_fn_dump;
extern const struct kn_function kn_fn_output;
extern const struct kn_function kn_fn_ascii;
extern const struct kn_function kn_fn_negate;
extern const struct kn_function kn_fn_box;
extern const struct kn_function kn_fn_head;
extern const struct kn_function kn_fn_tail;

#ifdef KN_EXT_EVAL
extern const struct kn_function kn_fn_eval;
#endif /* KN_EXT_EVAL */

#ifdef KN_EXT_SYSTEM
extern const struct kn_function kn_fn_system;
#endif /* KN_EXT_SYSTEM */

#ifdef KN_EXT_VALUE
/*
 * An extension function that converts its argument to a string, and then uses
 * the string's value as an identifier to look it up.
 *
 * `VALUE(+ "a" 23)` is simply equivalent to `EVAL(+ "a" 23)`, except the
 * parsing step of `EVAL` is skipped.
 *
 * Any lookups of non-variable-names (eg `VALUE(0)`) will simply terminate the
 * program like any unknown variable lookup would.
 */
extern const struct kn_function kn_fn_value;
#endif /* KN_EXT_VALUE */

/**
 *
 * 4.3 Arity 2
 *
 **/

extern const struct kn_function kn_fn_add;
extern const struct kn_function kn_fn_sub;
extern const struct kn_function kn_fn_mul;
extern const struct kn_function kn_fn_div;
extern const struct kn_function kn_fn_mod;
extern const struct kn_function kn_fn_pow;
extern const struct kn_function kn_fn_lth;
extern const struct kn_function kn_fn_gth;
extern const struct kn_function kn_fn_eql;
extern const struct kn_function kn_fn_and;
extern const struct kn_function kn_fn_or;
extern const struct kn_function kn_fn_then;
extern const struct kn_function kn_fn_assign;
extern const struct kn_function kn_fn_while;

/**
 *
 * 4.4 Arity 3
 *
 **/

extern const struct kn_function kn_fn_if;
extern const struct kn_function kn_fn_get;

/**
 *
 * 4.5 Arity 4
 *
 **/

extern const struct kn_function kn_fn_set;

#endif /* !KN_FUNCTION_H */
