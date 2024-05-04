#ifndef KN_AST_H
#define KN_AST_H

#include "allocator.h"
#include "function.h"
#include "value.h"
#include "shared.h"

#define KN_AST_FL_MARKED KN_GC_FL_MARKED

/**
 * The type that represents a function and its arguments in Knight.
 *
 * Note that this struct should be created through `kn_ast_alloc` and freed through `kn_ast_free`.
 **/
struct kn_ast {
	/**
	 * The allocation of the AST.
	 **/
	KN_HEADER

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
void KN_COLD kn_ast_cleanup(void);

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
void KN_COLD kn_ast_dealloc(struct kn_ast *ast);

#ifdef KN_USE_REFCOUNT
/**
 * clones the ast.
 **/
static inline struct kn_ast *kn_ast_clone(struct kn_ast *ast)  {
	++ast->refcount;
	return ast;
}

/**
 * Releases the memory resources associated with this struct.
 **/
static inline void kn_ast_free(struct kn_ast *ast)  {
	assert(ast->refcount != 0);

	if (--ast->refcount == 0)
		kn_ast_dealloc(ast);
}
#endif /* KN_USE_REFCOUNT */

#ifdef KN_USE_GC
void kn_ast_mark(struct kn_ast *ast);
#endif /* KN_USE_GC */

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
