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
	unsigned arity;

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
#define KN_DECLARE_FUNCTION(func_, arity_, name_)   \
	kn_value func_##_function(const kn_value *);    \
	const struct kn_function func_ = {              \
		.func = func_##_function,                   \
		.arity = arity_,                            \
		.name = name_                               \
	};                                              \
	kn_value func_##_function(const kn_value *args)

/******************************************************************************
 * The following are all of the different types of functions within Knight.   *
 * For details on what each function does, see the specs---these all conform. *
 ******************************************************************************/

/**
 * 4.1 Arity 0
 *
 * Note that the 4.1.1 (`TRUE`), 4.1.2 (`FALSE`) and 4.1.3 (`NULL`) aren't
 * implemented as functions, but rather literals.
 **/

/* 4.1.4 PROMPT */
extern const struct kn_function kn_fn_prompt;
kn_value kn_fn_prompt_function(const kn_value *args);

/* 4.1.5 RANDOM */
extern const struct kn_function kn_fn_random;
kn_value kn_fn_random_function(const kn_value *args);

/**
 * 4.2 Arity 1
 *
 * Note that the 4.2.1 (`:`) is treated as whitespace, and as such has no
 * associated function.
 **/

/* 4.2.2 EVAL */
extern const struct kn_function kn_fn_eval;
kn_value kn_fn_eval_function(const kn_value *args);

/* 4.2.3 BLOCK */
extern const struct kn_function kn_fn_block;
kn_value kn_fn_block_function(const kn_value *args);

/* 4.2.4 CALL */
extern const struct kn_function kn_fn_call;
kn_value kn_fn_call_function(const kn_value *args);

/* 4.2.5 ` */
extern const struct kn_function kn_fn_system;
kn_value kn_fn_system_function(const kn_value *args);

/* 4.2.6 QUIT */
extern const struct kn_function kn_fn_quit;
kn_value kn_fn_quit_function(const kn_value *args);

/* 4.2.7 ! */
extern const struct kn_function kn_fn_not;
kn_value kn_fn_not_function(const kn_value *args);

/* 4.2.8 LENGTH */
extern const struct kn_function kn_fn_length;
kn_value kn_fn_length_function(const kn_value *args);

/* 4.2.9 DUMP */
extern const struct kn_function kn_fn_dump;
kn_value kn_fn_dump_function(const kn_value *args);

/* 4.2.10 OUTPUT */
extern const struct kn_function kn_fn_output;
kn_value kn_fn_output_function(const kn_value *args);

/* 4.2.11 ASCII */
extern const struct kn_function kn_fn_ascii;
kn_value kn_fn_ascii_function(const kn_value *args);

/* 4.2.12 ~ */
extern const struct kn_function kn_fn_negate;
kn_value kn_fn_negate_function(const kn_value *args);

extern const struct kn_function kn_fn_box;
kn_value kn_fn_box_function(const kn_value *args);

extern const struct kn_function kn_fn_head;
kn_value kn_fn_head_function(const kn_value *args);

extern const struct kn_function kn_fn_tail;
kn_value kn_fn_tail_function(const kn_value *args);

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
kn_value kn_fn_value_function(const kn_value *args);
#endif /* KN_EXT_VALUE */

/**
 *
 * 4.3 Arity 2
 *
 **/

/* 4.3.1 + */
extern const struct kn_function kn_fn_add;
kn_value kn_fn_add_function(const kn_value *args);

/* 4.3.2 - */
extern const struct kn_function kn_fn_sub;
kn_value kn_fn_sub_function(const kn_value *args);

/* 4.3.3 * */
extern const struct kn_function kn_fn_mul;
kn_value kn_fn_mul_function(const kn_value *args);

/* 4.3.4 / */
extern const struct kn_function kn_fn_div;
kn_value kn_fn_div_function(const kn_value *args);

/* 4.3.5 % */
extern const struct kn_function kn_fn_mod;
kn_value kn_fn_mod_function(const kn_value *args);

/* 4.3.6 ^ */
extern const struct kn_function kn_fn_pow;
kn_value kn_fn_pow_function(const kn_value *args);

/* 4.3.7 < */
extern const struct kn_function kn_fn_lth;
kn_value kn_fn_lth_function(const kn_value *args);

/* 4.3.8 > */
extern const struct kn_function kn_fn_gth;
kn_value kn_fn_gth_function(const kn_value *args);

/* 4.3.9 ? */
extern const struct kn_function kn_fn_eql;
kn_value kn_fn_eql_function(const kn_value *args);

/* 4.3.10 & */
extern const struct kn_function kn_fn_and;
kn_value kn_fn_and_function(const kn_value *args);

/* 4.3.11 | */
extern const struct kn_function kn_fn_or;
kn_value kn_fn_or_function(const kn_value *args);

/* 4.3.12 ; */
extern const struct kn_function kn_fn_then;
kn_value kn_fn_then_function(const kn_value *args);

/* 4.3.13 = */
extern const struct kn_function kn_fn_assign;
kn_value kn_fn_assign_function(const kn_value *args);

/* 4.3.14 WHILE */
extern const struct kn_function kn_fn_while;
kn_value kn_fn_while_function(const kn_value *args);

/**
 *
 * 4.4 Arity 3
 *
 **/

/* 4.4.1 IF */
extern const struct kn_function kn_fn_if;
kn_value kn_fn_if_function(const kn_value *args);

/* 4.4.2 GET */
extern const struct kn_function kn_fn_get;
kn_value kn_fn_get_function(const kn_value *args);

/**
 *
 * 4.5 Arity 4
 *
 **/

/* 4.5.1 SUBSTITUTE */
extern const struct kn_function kn_fn_substitute;
kn_value kn_fn_substitute_function(const kn_value *args);

#endif /* !KN_FUNCTION_H */
