#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "list.h"
#include "ext.h"
#include "../src/custom.h"
#include "../src/shared.h"
#include "../src/function.h"
#include "../src/string.h"
#include "../src/parse.h"
#include "../src/ast.h"

#define container_of(ptr, type, member) ({                     \
        const typeof( ((type *)0)->member ) *_mptr = (ptr);    \
        (type *)( (char *)_mptr - offsetof(type,member) );})

#define ALLOC_CUSTOM_LIST(length) (kn_custom_alloc( \
		sizeof(struct kn_list) + sizeof(kn_value [length]), \
		&kn_list_vtable))

#define LIST(data) ((struct kn_list *) (data))

static bool is_idempotent(kn_value value) {
	return value == KN_NULL
		|| kn_value_is_boolean(value)
		|| kn_value_is_number(value)
		|| kn_value_is_string(value);
}

static void dump_list(void *data) {
	printf("List(");
	
	for (size_t i = 0; i < LIST(data)->length; ++i) {
		if (i != 0)
			printf(", ");

		kn_value_dump(LIST(data)->elements[i]);
	}

	printf(")");
}

static void free_list(void *data) {
	for (size_t i = 0; i < LIST(data)->length; ++i) 
		kn_value_free(LIST(data)->elements[i]);
}

// just run through the LIST and evaluate each argument.
static void run_list(struct kn_list *list) {	
	if (list->idempotent)
		return;

	for (size_t i = 0; i < list->length; ++i)
		kn_value_free(kn_value_run(list->elements[i]));
}

static kn_number list_custom_to_number(void *data) {
	run_list(LIST(data));

	return (kn_number) LIST(data)->length;
}

static kn_boolean list_custom_to_boolean(void *data) {
	run_list(LIST(data));

	return LIST(data)->length != 0;
}

static struct kn_string *list_custom_to_string(void *data) {
	size_t capacity = 1024;
	char *str = xmalloc(capacity);
	size_t length = 0;

	str[length++] = '[';

	for (size_t i = 0; i < LIST(data)->length; ++i) {
		if (i != 0) {
			memcpy(str + length, ", ", 2);
			length += 2;
		}

		struct kn_string *string = kn_value_to_string(LIST(data)->elements[i]);

		if (capacity <= length + kn_string_length(string) + 2)
			str = xrealloc(str, capacity *= 2);

		memcpy(str + length, kn_string_deref(string), kn_string_length(string));

		length += kn_string_length(string);
	}

	str[length++] = ']';
	str[length] = '\0';
	return kn_string_new_owned(str, length);
}

static kn_value kn_list_run(void *data) {
	struct kn_list *list = (struct kn_list *) data;

	if (list->idempotent) {
		struct kn_custom *custom = container_of(data, struct kn_custom, data);

		return kn_value_new_custom(kn_custom_clone(custom));
	}

	struct kn_custom *custom = ALLOC_CUSTOM_LIST(list->length);

	LIST(custom->data)->length = list->length;
	LIST(custom->data)->idempotent = true; // if it wasn't, we'd returned.

	for (size_t i = 0; i < list->length; ++i)
		LIST(custom->data)->elements[i] = kn_value_run(list->elements[i]);

	return kn_value_new_custom(custom);
}

const struct kn_custom_vtable kn_list_vtable = {
	.free = free_list,
	.dump = dump_list,
	.run = kn_list_run,
	.to_number = list_custom_to_number,
	.to_boolean = list_custom_to_boolean,
	.to_string = list_custom_to_string
};


KN_FUNCTION_DECLARE(xadd, 2, "X+") {
	kn_value lhs = kn_value_run(args[0]);
	kn_value rhs = kn_value_run(args[1]);

	struct kn_list *llist = LIST(kn_value_as_custom(lhs)->data);
	struct kn_list *rlist = LIST(kn_value_as_custom(rhs)->data);

	size_t length = llist->length + rlist->length;

	struct kn_custom *custom = ALLOC_CUSTOM_LIST(length);

	LIST(custom->data)->length = length;
	LIST(custom->data)->idempotent = llist->idempotent || rlist->idempotent;

	size_t i;

	for (i = 0; i < llist->length; ++i)
		LIST(custom->data)->elements[i] = kn_value_run(llist->elements[i]);

	for (size_t j = 0; j < rlist->length; ++j, ++i)
		LIST(custom->data)->elements[i] = kn_value_run(rlist->elements[j]);

	return kn_value_new_custom(custom);
}

KN_FUNCTION_DECLARE(xlast, 1, "X_LAST") {
	kn_value ran = kn_value_run(args[0]);

	struct kn_list *list = LIST(kn_value_as_custom(ran)->data);

	if (list->length == 0) die("cannot get last of empty list");

	return kn_value_run(list->elements[list->length - 1]);
}

KN_FUNCTION_DECLARE(xpush, 2, "X_PUSH") {
	kn_value ran = kn_value_run(args[0]);
	kn_value element = kn_value_run(args[1]);

	struct kn_list *list = LIST(kn_value_as_custom(ran)->data);
	struct kn_custom *custom = ALLOC_CUSTOM_LIST(list->length + 1);

	LIST(custom->data)->length = list->length + 1;
	LIST(custom->data)->idempotent = list->idempotent && is_idempotent(element);

	for (size_t i = 0; i < list->length; ++i)
		LIST(custom->data)->elements[i] = kn_value_run(list->elements[i]);

	LIST(custom->data)->elements[list->length] = element;

	return kn_value_new_custom(custom);
}

KN_FUNCTION_DECLARE(xpop, 1, "X_POP") {
	kn_value ran = kn_value_run(args[0]);

	struct kn_list *list = LIST(kn_value_as_custom(ran)->data);
	if (list->length == 0) die("cannot pop from empty list");

	size_t length = list->length - 1;
	struct kn_custom *custom = ALLOC_CUSTOM_LIST(length);
	LIST(custom->data)->length = length;

	if (list->idempotent) {
		LIST(custom->data)->idempotent = true;
	} else if (is_idempotent(list->elements[length])) {
		// pop an idempotent on, we still got a nonidempotent somewhere
		LIST(custom->data)->idempotent = false;
	} else {
		for (size_t i = 0; i < list->length; ++i) {
			if (!is_idempotent(list->elements[i])) {
				LIST(custom->data)->idempotent = false;
				goto done;
			}
		}

		LIST(custom->data)->idempotent = true;
	done:
		;
	}

	for (size_t i = 0; i < length; ++i)
		LIST(custom->data)->elements[i] = kn_value_run(list->elements[i]);

	return kn_value_new_custom(custom);

	// LIST(custom->data)->length = list->length + 1;
	// LIST(custom->data)->idempotent = list->idempotent && is_idempotent(element);

	// for (size_t i = 0; i < list->length; ++i)
	// 	LIST(custom->data)->elements[i] = kn_value_run(list->elements[i]);

	// LIST(custom->data)->elements[list->length] = element;

	return kn_value_new_custom(custom);
}

KN_FUNCTION_DECLARE(xget, 2, "X_GET") {
	kn_value ran = kn_value_run(args[0]);
	size_t index = (size_t) kn_value_to_number(args[1]);

	struct kn_list *list = LIST(kn_value_as_custom(ran)->data);

	if (list->length <= index)
		die("index %zu is too large (length %zu)", index, list->length);

	kn_value ret = kn_value_clone(list->elements[index]);
	kn_value_free(ran);
	return ret;
}

KN_FUNCTION_DECLARE(xget, 2, "X_SET") {
	kn_value ran = kn_value_run(args[0]);
	size_t index = (size_t) kn_value_to_number(args[1]);

	struct kn_list *list = LIST(kn_value_as_custom(ran)->data);

	if (list->length <= index)
		die("index %zu is too large (length %zu)", index, list->length);

	kn_value ret = kn_value_clone(list->elements[index]);
	kn_value_free(ran);
	return ret;
}


static unsigned depth;

struct kn_custom *parse_list() {
	unsigned current_depth = depth++;
	size_t capacity = 16; // unlikely to have more than 16 in a literal.

	struct kn_custom *custom = ALLOC_CUSTOM_LIST(capacity);

	LIST(custom->data)->length = 0;
	LIST(custom->data)->idempotent = true;
	LIST(custom->data)->elements
	kn_value parsed;

	while (depth != current_depth) {
		parsed = kn_parse_value();
		LIST(custom->data)->elements[LIST(custom->data)->length++] = parsed;

		if (parsed == KN_UNDEFINED) die("missing closing 'X]'");

		if (!is_idempotent(parsed))
			LIST(custom->data)->idempotent = false;

		if (LIST(custom->data)->length == capacity) {
			capacity *= 2;
			custom = xrealloc(
				custom,
				sizeof(struct kn_custom)
					+ sizeof(struct kn_list)
					+ sizeof(kn_value [capacity])
			);
		}
	}

	if (current_depth != depth) 
		die("missing closing 'X]'");

	return xrealloc(
		custom,
		sizeof(struct kn_custom)
			+ sizeof(struct kn_list)
			+ sizeof(kn_value [capacity])
	);
}

kn_value kn_parse_extension_list() {
	switch (kn_parse_peek()) {
	case '[':
		return kn_value_new_custom(parse_list());

	case ']':
		if (!depth--)
			die("unexpected `X]`");
		return KN_NULL;

	case 'G':
		if (!stream_starts_with_strip("GET"))
			return KN_UNDEFINED;

		return kn_value_new_ast(kn_parse_ast(&kn_fn_xget));

	case 'L':
		if (!stream_starts_with_strip("LAST"))
			return KN_UNDEFINED;

		return kn_value_new_ast(kn_parse_ast(&kn_fn_xlast));

	case 'P': {
		if (!stream_starts_with("POP") && !stream_starts_with("PUSH"))
			return KN_UNDEFINED;
		char c = kn_parse_peek_advance();
		strip_keyword();

		return kn_value_new_ast(
			kn_parse_ast(c == 'U' ? &kn_fn_xpush : &kn_fn_xpop)
		);
	}

	case '+': 
		return kn_value_new_ast(kn_parse_ast(&kn_fn_xadd));

	default:
		return KN_UNDEFINED;
	}
}



