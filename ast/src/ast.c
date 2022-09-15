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
	for (size_t i = 0; i <= KN_MAX_ARGC; ++i) {
		for (size_t j = 0; j < KN_AST_FREE_CACHE_LEN; ++j) {
			struct kn_ast *ast = freed_asts[i][j];

			if (ast != NULL) {
				assert(ast->refcount == 0);
				free(ast);
			}
		}
	}
#endif /* KN_AST_CACHE */
}

struct kn_ast *kn_ast_alloc(size_t argc) {
	struct kn_ast *ast;

#ifdef KN_AST_CACHE
	assert(argc <= KN_MAX_ARGC);

	// Try to find a freed ast we can repurpose.
	for (size_t i = 0; i < KN_AST_FREE_CACHE_LEN; ++i) {
		ast = freed_asts[argc][i];

 		// If it's null, then we can't repurpose it.
		if (ast == NULL)
			continue;

		// Don't let others use this one.
		freed_asts[argc][i] = NULL;

		// Sanity check.
		assert(ast->refcount == 0);

		// Increase the refcount as we're now using it.
		++ast->refcount;
		return ast;
	}
#endif /* KN_AST_CACHE */

	// There are no cached free asts, so we have to allocate.
	ast = xmalloc(sizeof(struct kn_ast) + sizeof(kn_value) * argc);
	ast->refcount = 1;

	return ast;
}

void kn_ast_deallocate(struct kn_ast *ast) {
	assert(ast->refcount == 0);

	size_t arity = ast->func->arity;

	// Free all arguments associated with this ast.
	for (size_t i = 0; i < arity; ++i)
		kn_value_free(ast->args[i]);

#ifdef KN_AST_CACHE
	// Attempt to cache this ast, so another allocation can reuse its space.
	for (size_t i = 0; i < KN_AST_FREE_CACHE_LEN; ++i) {
		// If the freed slot is unused, claim it.
		if (freed_asts[arity][i] == NULL) {
			freed_asts[arity][i] = ast;
			return;
		}
	}
#endif /* KN_AST_CACHE */
	
	// All free slots are used, we cannot repurpose it.
	free(ast);
}

void kn_ast_dump(const struct kn_ast *ast, FILE *out) {
	fputs("Function(", out);
	fputs(ast->func->name, out);

	kn_indentation++;
	for (size_t i = 0; i < ast->func->arity; ++i) {
		fputs(", ", out);
		kn_indent(out);
		kn_value_dump(ast->args[i], out);
	}

	kn_indentation--;
	kn_indent(out);
	fputc(')', out);
}
