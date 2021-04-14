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

struct kn_list kn_list_empty = { .length = 0, .idempotent = true };

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

// just run through the list and evaluate each argument.
static void run_list(struct kn_list *list) {	
	if (list->idempotent)
		return;

	for (size_t i = 0; i < list->length; ++i)
		kn_value_free(kn_value_run(list->elements[i]));
}

static kn_number list_custom_to_number(void *data) {
	struct kn_list *list = (struct kn_list *) data;

	run_list(list);

	return (kn_number) list->length;
}

static kn_boolean list_custom_to_boolean(void *data) {
	run_list(data);

	return data != &kn_list_empty;
}

static struct kn_string *list_custom_to_string(void *data) {
	// todo: list to string
	(void *) data;
	return kn_string_new_owned(strdup("todo1"), 5);
}

const struct kn_custom_vtable kn_list_vtable = {
	.free = (void (*) (void *)) kn_list_free,
	.dump = (void (*) (void *)) kn_list_dump,
	.run = kn_list_run,
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

struct kn_list *parse_list() {
	unsigned current_depth = depth++;
	size_t capacity = 16; // unlikely to have more than 16 in a literal.

	struct kn_list *list = kn_list_alloc(capacity);
	list->length = 0;
	list->idempotent = true;
	kn_value parsed;

	while ((parsed = kn_parse_value()) != KN_UNDEFINED) {
		list->elements[list->lengthe++] = parsed;

		if (parsed != KN_TRUE && parsed != KN_FALSE && parsed != KN_NULL
			&& !kn_value_is_number(parsed) && !kn_value_is_string(number)
		) {
			list->idempotent = false;
		}

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

kn_value kn_parse_extension() {
	const struct kn_function *function;

	char fn_chr, c;

	while ((fn_chr = kn_parse_peek_advance()) == '_') {
		/* do nothing, strip leading `_`s. */
	}

	while (('A' <= (c = kn_parse_peek()) && c <= 'Z') || c == '_')
		kn_parse_advance();

	switch (fn_chr) {
	case '[':
		return kn_value_new_custom(parse_list(), &kn_list_vtable);

	case ']':
		if (!depth--)
			die("unexpected `X]`");
		return KN_UNDEFINED;

	case 'G': 
		return kn_value_new_ast(kn_parse_ast(&kn_fn_xget));

	default:
		die("unknown extension character '%c'", kn_parse_stream[-1]);
	}
}



