#include "ast.h"
#include <stdlib.h>
#include <assert.h>

#ifdef KN_AST_CACHE
# ifndef KN_AST_FREE_CACHE_LEN
#  define KN_AST_FREE_CACHE_LEN 32
# endif /* !KN_AST_FREE_CACHE_LEN */
static struct kn_ast *freed_asts[KN_MAX_ARGC + 1][KN_AST_FREE_CACHE_LEN];
#endif /* KN_AST_CACHE */

void kn_ast_cleanup(void) {
#if defined(KN_AST_CACHE) && defined(KN_USE_REFCOUNT)
	for (size_t i = 0; i <= KN_MAX_ARGC; ++i) {
		for (size_t j = 0; j < KN_AST_FREE_CACHE_LEN; ++j) {
			struct kn_ast *ast = freed_asts[i][j];

			if (ast != NULL) {
				assert(ast->refcount == 0);
				kn_heap_free(ast);
			}
		}
	}
#endif /* KN_AST_CACHE && KN_USE_REFCOUNT */
}

#ifdef KN_USE_GC
void kn_ast_mark(struct kn_ast *ast) {
	kn_flags(ast) |= KN_AST_FL_MARKED;

	for (size_t i = 0; i < ast->function->arity; ++i)
		kn_value_mark(ast->args[i]);
}
#endif /* KN_USE_GC */

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

# ifdef KN_USE_REFCOUNT
		// Sanity check.
		assert(ast->refcount == 0);

		// Increase the refcount as we're now using it.
		++ast->refcount;
# endif /* KN_USE_REFCOUNT */

		return ast;
	}
#endif /* KN_AST_CACHE */

	// There are no cached free asts, so we have to allocate.
	ast = kn_heap_malloc(sizeof(struct kn_ast) + sizeof(kn_value) * argc);
	// ast = kn_gc_malloc(struct kn_ast);

#ifdef KN_USE_REFCOUNT
	ast->refcount = 1;
#endif /* KN_USE_REFCOUNT */

	return ast;
}

void kn_ast_dealloc(struct kn_ast *ast) {
#ifdef KN_USE_REFCOUNT
	assert(ast->refcount == 0);
#endif /* KN_USE_REFCOUNT */

	size_t arity = ast->function->arity;

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
	kn_heap_free(ast);
}

void kn_ast_dump(const struct kn_ast *ast, FILE *out) {
	fputs("AST(", out);
	fputs(ast->function->name, out);

	for (size_t i = 0; i < ast->function->arity; ++i) {
		fputs(", ", out);
		kn_value_dump(ast->args[i], out);
	}

	fputc(')', out);
}
