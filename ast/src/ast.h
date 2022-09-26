#ifndef KN_AST_H
#define KN_AST_H

#include "refcount.h"
#include "function.h"
#include "value.h"
#include "shared.h"

/**
 * The type that represents a function and its arguments in Knight.
 *
 * Note that this struct should be created through `kn_ast_alloc` and freed through `kn_ast_free`.
 **/
struct kn_ast {
	struct kn_refcount refcount;

	/*
	 * The function associated with this ast.
	 */
	const struct kn_function *function;

	/*
	 * The arguments of this ast.
	 */
	kn_value args[];
};

/**
 * Frees memory associated with zombie ASTs.
 **/
void KN_ATTRIBUTE(cold) kn_ast_cleanup(void);

/**
 * Allocates a new `kn_ast` with the given number of arguments.
 *
 * `argc` shouldn't be larger than `KN_MAX_ARGC`.
 **/
struct kn_ast *kn_ast_alloc(size_t argc);

/**
 * Deallocates the memory associated with `ast`; should only be called with
 * an ast with a zero refcount.
 **/
void KN_ATTRIBUTE(cold) kn_ast_dealloc(struct kn_ast *ast);

/**
 * Releases the memory resources associated with this struct.
 **/
static inline void kn_ast_free(struct kn_ast *ast) {
	assert(*kn_refcount(ast) != 0);

	if (--*kn_refcount(ast) == 0)
		kn_ast_dealloc(ast);
}

/**
 * Executes a `kn_ast`, returning the function's result.
 **/
static inline kn_value kn_ast_run(const struct kn_ast *ast) {
	return (ast->function->func)(ast->args);
}

/**
 * Dumps a debugging representation of `ast` to `out`.
 **/
void kn_ast_dump(const struct kn_ast *ast, FILE *out);

#endif /* !KN_AST_H */
