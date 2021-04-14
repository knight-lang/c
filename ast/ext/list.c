#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "../src/custom.h"
#include "../src/shared.h"
#include "../src/function.h"
#include "../src/string.h"
#include "../src/parse.h"
#include "../src/ast.h"

struct kn_list kn_list_empty = { .length = 0 };

#define KNLISTLEN(length) (sizeof(struct kn_list) + sizeof(kn_value [length]))
struct kn_list *kn_list_alloc(size_t capacity) {
	if (capacity == 0)
		return &kn_list_empty;

	return xmalloc(KNLISTLEN(capacity));
}

void kn_list_free(struct kn_list *list) {
	free(list);
}

void kn_list_dump(const struct kn_list *list) {
	printf("List(");
	
	for (size_t i = 0; i < list->length; ++i) {
		if (i != 0)
			printf(", ");

		kn_value_dump(list->elements[i]);
	}

	printf(")");
}

static kn_number list_custom_to_number(void *data) {
	return (kn_number) ((struct kn_list *) data)->length;
}


static kn_boolean list_custom_to_boolean(void *data) {
	return data != &kn_list_empty;
}

static struct kn_string *list_custom_to_string(void *data) {
	printf("dumped!\n");
	return kn_string_new_owned(strdup("todo1"), 5);
}

const struct kn_custom_vtable kn_list_vtable = {
	.free = (void (*) (void *)) kn_list_free,
	.dump = (void (*) (void *)) kn_list_dump,
	.run = NULL,
	.to_number = list_custom_to_number,
	.to_boolean = list_custom_to_boolean,
	.to_string = list_custom_to_string
};

KN_FUNCTION_DECLARE(xget, 2, "X_GET") {
	kn_value run = kn_value_run(args[0]);
	size_t index = (size_t) kn_value_to_number(args[1]);

	struct kn_list *list = (struct kn_list *) kn_value_as_custom(run)->data;

	if (list->length <= index)
		die("index %zu is too large (length %zu)", index, list->length);

	kn_value ret = kn_value_clone(list->elements[index]);
	kn_value_free(run);
	return ret;
}

// KN_FUNCTION_DECLARE(cdr, 1, 'D') {
// 	kn_value run = kn_value_run(args[0]);
// 	struct kn_list *list = (struct kn_list *) kn_value_as_custom(run);
// 	struct kn_list *ret = kn_list_clone(kn_list_cdr(list));

// 	kn_value_free(run);

// 	return kn_value_new_custom(ret, &kn_list_vtable);
// }

// KN_FUNCTION_DECLARE(cons, 2, 'C') {
// 	kn_value run = kn_value_run(args[0]);
// 	struct kn_list *list = (struct kn_list *) kn_value_as_custom(run);
// 	struct kn_list *ret = kn_list_clone(kn_list_cdr(list));

// 	kn_value_free(run);

// 	return kn_value_new_custom(ret, &kn_list_vtable);
// }

static unsigned depth;

struct kn_list *parse_list(const char **stream) {
	unsigned current_depth = depth++;
	size_t capacity = 16; // unlikely to have more than 16 in a literal.

	struct kn_list *list = kn_list_alloc(capacity);
	list->length = 0;
	kn_value parsed;

	while ((parsed = kn_parse(stream)) != KN_UNDEFINED) {
		list->elements[list->length++] = parsed;

		if (list->length == capacity) {
			capacity *= 2;
			list = xrealloc(list, KNLISTLEN(capacity));
		}
	}

	if (current_depth != depth) 
		die("missing closing 'X]'");

	if (list->length == 0) {
		kn_list_free(list);
		return &kn_list_empty;
	} else {
		return xrealloc(list, KNLISTLEN(list->length));
	}
}

kn_value kn_parse_extension(const char **stream) {
	const struct kn_function *function;

	while (**stream == '_') ++*stream; // ignore leading `_`s.

	switch (*(*stream)++) {
	case '[':
		return kn_value_new_custom(parse_list(stream), &kn_list_vtable);

	case ']':
		if (!depth--)
			die("unexpected `X]`");
		return KN_UNDEFINED;
	case 'G': 
		function = &kn_fn_xget;
		goto parse_function;
	// case 'D':
	// 	function = &kn_fn_cdr;
	// 	goto parse_function;
	// case 'C':
	// 	function = &kn_fn_cons;

	parse_function: {
		struct kn_ast *ast = kn_ast_alloc(function->arity);

		ast->func = function;
		ast->refcount = 1;

		for (size_t i = 0; i < function->arity; ++i) {
			if ((ast->args[i] = kn_parse(stream)) == KN_UNDEFINED) {
				die("unable to parse argument %zu for function '%s'",
					i, function->name);
			}
		}

		return kn_value_new_ast(ast);
	}

	default:
		die("unknown extension character '%c'", (*stream)[-1]);
	}
}



