#include "ast.h"    /* prototypes, kn_ast, kn_value, kn_value_free */
#include "shared.h" /* xmalloc_value_aligned */
#include <stdlib.h> /* free */

struct kn_ast *kn_ast_alloc(unsigned argc) {
	if ((argc * sizeof(kn_value)) % 16)
		++argc; // adjust alignment for `xmalloc_value_aligned`.

	struct kn_ast *ast = xmalloc_value_aligned(
		sizeof(struct kn_ast) + sizeof(kn_value [argc])
	);

	ast->refcount = 1;
	return ast;
}

struct kn_ast *kn_ast_clone(struct kn_ast *ast) {
	++ast->refcount;

	return ast;
}

void kn_ast_free(struct kn_ast *ast) {
	if (--ast->refcount) // if we're not the last reference, leave early.
		return;

	for (unsigned i = 0; i < ast->func->arity; ++i)
		kn_value_free(ast->args[i]);

	free(ast);
}

kn_value kn_ast_run(struct kn_ast *ast) {
	return (ast->func->func)(ast->args);
}
