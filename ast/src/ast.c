#include "ast.h"    /* prototypes, kn_ast, kn_value, kn_value_free,
                       KN_MAX_ARGC */
#include "shared.h" /* xmalloc, KN_UNLIKELY */
#include <stdlib.h> /* free, NULL */
#include <assert.h> /* assert */

#ifdef KN_AST_CACHE
# ifndef KN_AST_FREE_CACHE_LEN
#  define KN_AST_FREE_CACHE_LEN 32
# endif /* !KN_AST_FREE_CACHE_LEN */

struct kn_ast *freed_asts[KN_MAX_ARGC + 1][KN_AST_FREE_CACHE_LEN];
#endif /* KN_AST_CACHE */

void kn_ast_cleanup(void) {
#ifdef KN_AST_CACHE
	struct kn_ast *ast;

	for (unsigned i = 0; i <= KN_MAX_ARGC; ++i) {
		for (unsigned j = 0; j < KN_AST_FREE_CACHE_LEN; ++j) {
			ast = freed_asts[i][j];

			if (ast != NULL) {
				assert(ast->refcount == 0);
				free(ast);
			}
		}
	}
#endif /* KN_AST_CACHE */
}

struct kn_ast *kn_ast_alloc(unsigned argc) {
	struct kn_ast *ast;

	assert(argc <= KN_MAX_ARGC);

#ifdef KN_AST_CACHE
	// try to find a freed ast we can repurpose.
	for (unsigned i = 0; i < KN_AST_FREE_CACHE_LEN; ++i) {
		ast = freed_asts[argc][i];

 		// if it's null, then we can't repurpose it.
		if (ast == NULL)
			continue;

		// dont let others use this one.
		freed_asts[argc][i] = NULL;

		// sanity check
		assert(ast->refcount == 0);

		// increase the refcount as we're now using it.
		++ast->refcount;
		return ast;
	}
#endif /* KN_AST_CACHE */

	// there are no cached free asts, so we have to allocate.
	ast = xmalloc(sizeof(struct kn_ast) + sizeof(kn_value [argc]));
	ast->refcount = 1;

	return ast;
}

struct kn_ast *kn_ast_clone(struct kn_ast *ast) {
	++ast->refcount;

	return ast;
}

void kn_ast_free(struct kn_ast *ast) {
	assert(ast->refcount != 0);

	// if we're static, dont attempt to free us.
	if (KN_UNLIKELY(ast->refcount < 0))
		return;

	// if we're not the last reference, leave early.
	if (--ast->refcount)
		return;

	unsigned arity = ast->func->arity;

	// free all arguments associated with this ast.
	for (unsigned i = 0; i < arity; ++i)
		kn_value_free(ast->args[i]);

#ifdef KN_AST_CACHE
	// attempt to cache this ast, so another allocation can reuse its space.
	for (unsigned i = 0; i < KN_AST_FREE_CACHE_LEN; ++i) {
		// if the freed slot is unused, claim it.
		if (freed_asts[arity][i] == NULL) {
			freed_asts[arity][i] = ast;
			return;
		}
	}
#endif /* KN_AST_CACHE */
	
	// all free slots are used, we cannot repurpose it.
	free(ast);
}

kn_value kn_ast_run(struct kn_ast *ast) {
	return (ast->func->func)(ast->args);
}
