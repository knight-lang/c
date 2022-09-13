#ifndef KN_AST_H
#define KN_AST_H

#include "function.h" /* kn_function, KN_MAX_ARGC */
#include "value.h"    /* kn_value */
#include <stdalign.h> /* alignas */

/*
 * The type that represents a function and its arguments in Knight.
 *
 * Note that this struct should be passed to `kn_ast_free` to release its
 * resources.
 */
struct kn_ast {
	/*
	 * How many references to this object exist.
	 */
	alignas(8) unsigned refcount;

    /*
     * Whether or not we're static.
     */
    unsigned is_static;

	/*
	 * The function associated with this ast.
	 */
	const struct kn_function *func;

	/*
	 * The arguments of this ast.
	 */
	kn_value args[];
};

/*
 * Frees memory associated with zombie ASTs.
 */
void kn_ast_cleanup(void);

/*
 * Allocates a new `kn_ast` with the given number of arguments.
 *
 * `argc` musn't be larger than `KN_MAX_ARGC`.
 */
struct kn_ast *kn_ast_alloc(unsigned argc);

/*
 * Duplicates the ast, returning a new value that must be `kn_ast_free`d
 * independently from the passed `ast`.
 */
static inline struct kn_ast *kn_ast_clone(struct kn_ast *ast) {
    assert(ast->refcount);
    ast->refcount++;
    return ast;
}

/*
 * Deallocates the memory associated with `ast`; should only be called with
 * an ast with a zero refcount.
 */
void kn_ast_deallocate(struct kn_ast *ast);

/*
 * Releases the memory resources associated with this struct.
 */
static inline void kn_ast_free(struct kn_ast *ast) {
    assert(ast->refcount);
    if (--ast->refcount == 0)
        kn_ast_deallocate(ast);
}

/*
 * Executes a `kn_ast`, returning the value associated with its execution.
 */
static inline kn_value kn_ast_run(struct kn_ast *ast) {
    return (ast->func->func)(ast->args);
}

void kn_ast_dump(const struct kn_ast *ast, FILE *out);

#endif /* !KN_AST_H */
