#include "list.h"
#include "ext.h"
#include "../src/shared.h"
#include "../src/string.h"
#include "../src/function.h"
#include "../src/parse.h"
#include <string.h>
#include <assert.h>

#define LIST(data) ((struct list *) data)
#define ALLOC_CUSTOM_LIST() \
	(kn_custom_alloc(sizeof(struct list), &list_vtable))

static void list_realloc(struct list *list, size_t newlength) {
	list->elements = xrealloc(list->elements, sizeof(kn_value [newlength]));
}

kn_value list_pop(struct list *list) {
	return list_is_empty(list) ? KN_UNDEFINED : list->elements[--list->length];
}

void list_push(struct list *list, kn_value value) {
	if (list->length == list->capacity)
		list_realloc(list, list->capacity *= 2);

	list->elements[list->length++] = value;
}

kn_value list_get(const struct list *list, size_t index) {
	return (list->length <= index) ? KN_UNDEFINED : list->elements[index];
}

void list_resize(struct list *list, size_t index) {
	if (list->capacity <= index) {
		list_realloc(list, list->capacity = index + 1);
		assert(list->length <= index);
	}

	if (list->length <= index)
		for (size_t i = list->length; i <= index; ++i)
			list->elements[i] = KN_NULL;

	list->length = index + 1;
}

void list_set(struct list *list, size_t index, kn_value value) {
	if (index == list->length) {
		list_push(list, value);
		return;
	}

	if (list->length <= index) {
		list_realloc(list, list->capacity = index + 1);

		for (size_t i = list->length; i <= index; ++i)
			list->elements[i] = KN_NULL;
	}

	kn_value_free(list->elements[index]);
	list->elements[index] = value;

}

void list_insert(struct list *list, size_t index, kn_value value) {
	(void ) list;
	(void ) index;
	(void ) value;
	die("todo: insert");
}

kn_value list_delete(struct list *list, size_t index) {
	if (list->length <= index) return KN_UNDEFINED;
	if (index == list->length - 1) return list_pop(list);

	kn_value element = list->elements[index];

	memmove(list->elements + (index - 1),
		list->elements + index, list->length - index);

	return element;
}

void list_free(struct list *list) {
	for (size_t i = 0; i < list->length; ++i)
		kn_value_free(list->elements[i]);
}

void list_dump(const struct list *list) {
	printf("List(");
	
	for (size_t i = 0; i < list->length; ++i) {
		if (i != 0) {
			putchar(',');
			putchar(' ');
		}

		kn_value_dump(list->elements[i]);
	}

	putchar(')');
}

bool list_is_empty(const struct list *list) {
	// todo: empty list.
	return list->length == 0;
}

struct kn_string *list_to_string(struct list *list) {
	size_t capacity = 1024;
	char *str = xmalloc(capacity);
	size_t length = 0;

	str[length++] = '[';

	for (size_t i = 0; i < list->length; ++i) {
		if (i != 0) {
			memcpy(str + length, ", ", 2);
			length += 2;
		}

		struct kn_string *string = kn_value_to_string(list->elements[i]);

		if (capacity <= length + kn_string_length(string) + 2)
			str = xrealloc(str, capacity *= 2);

		memcpy(str + length, kn_string_deref(string), kn_string_length(string));

		length += kn_string_length(string);
	}

	str[length++] = ']';
	str[length] = '\0';
	return kn_string_new_owned(str, length);
}

#define DEFAULT_CAPACITY 16

kn_value list_run(struct list *list) {
	if (list->idempotent) {
		struct kn_custom *custom
			= container_of((void *) list, struct kn_custom, data);

		return kn_value_new_custom(kn_custom_clone(custom));
	}

	struct kn_custom *custom = ALLOC_CUSTOM_LIST();

	struct list *ran = LIST(custom->data);

	ran->elements = xmalloc(sizeof(kn_value [DEFAULT_CAPACITY]));
	ran->length = list->length;
	ran->capacity = list->capacity;
	ran->idempotent = list->idempotent;

	for (size_t i = 0; i < list->length; ++i)
		ran->elements[i] = kn_value_run(list->elements[i]);

	return kn_value_new_custom(custom);
}

struct kn_custom *list_concat(struct list *lhs, struct list *rhs) {
	size_t length = lhs->length + rhs->length;

	struct kn_custom *custom = ALLOC_CUSTOM_LIST();
	struct list *list = LIST(custom->data);

	list->length = length;
	list->elements = xmalloc(sizeof(kn_value [length]));
	list->idempotent = lhs->idempotent && rhs->idempotent;

	size_t i;

	for (i = 0; i < lhs->length; ++i)
		list->elements[i] = kn_value_run(lhs->elements[i]);

	for (size_t j = 0; j < rhs->length; ++j, ++i)
		list->elements[i] = kn_value_run(rhs->elements[j]);

	return custom;

}

static bool is_idempotent(kn_value value) {
	return value == KN_NULL
		|| kn_value_is_boolean(value)
		|| kn_value_is_number(value)
		|| kn_value_is_string(value);
}

static unsigned depth;

static kn_value parse_list() {
	unsigned current_depth = depth++;

	struct kn_custom *custom
		= kn_custom_alloc(sizeof(struct list), &list_vtable);

	struct list *list = LIST(custom->data);

	list->elements = xmalloc(sizeof(kn_value [DEFAULT_CAPACITY]));
	list->length = 0;
	list->capacity = DEFAULT_CAPACITY;
	list->idempotent = true;

	while (1) {
		kn_value parsed = kn_parse_value();

		if (depth == current_depth) {
			assert(parsed == KN_NULL);
			break;
		}

		if (parsed == KN_UNDEFINED)
			die("missing closing 'X]'");

		list->elements[list->length++] = parsed;

		if (!is_idempotent(parsed))
			list->idempotent = false;

		if (list->length == list->capacity) {
			list->capacity *= 2;
			list->elements =
				xrealloc(list->elements, sizeof(kn_value [list->capacity]));
		}
	}

	if (current_depth != depth) 
		die("missing closing 'X]'");

	list->elements =
		xrealloc(list->elements, sizeof(kn_value [list->capacity]));

	return kn_value_new_custom(custom);
}

#define VALUE2LIST(value) LIST(kn_value_as_custom(value)->data)
#define CORRECT_INDEX(index) (index < 0 ? (VALUE2LIST(ran)->length + index) : index)

KN_DECLARE_FUNCTION(list_pop_fn, 1, "X_LPOP") {
	kn_value ran = kn_value_run(args[0]);
	kn_value popped = list_pop(VALUE2LIST(ran));
	kn_value_free(ran);

	// pop from an empty list yields null
	return popped == KN_UNDEFINED ? KN_NULL : popped;
}

KN_DECLARE_FUNCTION(list_push_fn, 2, "X_LPUSH") {
	kn_value ran = kn_value_run(args[0]);
	list_push(VALUE2LIST(ran), kn_value_run(args[1]));
	return ran;
}

KN_DECLARE_FUNCTION(list_get_fn, 2, "X_LGET") {
	kn_value ran = kn_value_run(args[0]);
	kn_number index = kn_value_to_number(args[1]);
	kn_value retrieved = list_get(VALUE2LIST(ran), CORRECT_INDEX(index));
	kn_value_free(ran);

	return retrieved == KN_UNDEFINED ? KN_NULL : kn_value_clone(retrieved);
}

KN_DECLARE_FUNCTION(list_set_fn, 3, "X_LSET") {
	kn_value ran = kn_value_run(args[0]);
	kn_value value = kn_value_run(args[1]);
	kn_number index = kn_value_to_number(args[2]);
	list_set(VALUE2LIST(ran), CORRECT_INDEX(index), value);

	return ran;
}

KN_DECLARE_FUNCTION(list_insert_fn, 3, "X_LINSERT") {
	kn_value ran = kn_value_run(args[0]);
	kn_value value = kn_value_run(args[1]);
	kn_number index = kn_value_to_number(args[2]);
	list_insert(VALUE2LIST(ran), CORRECT_INDEX(index), value);

	return ran;
}

KN_DECLARE_FUNCTION(list_concat_fn, 2, "X+") {
	kn_value lhs = kn_value_run(args[0]);
	kn_value rhs = kn_value_run(args[1]);
	kn_value ret = kn_value_new_custom(list_concat(VALUE2LIST(lhs), VALUE2LIST(rhs)));

	kn_value_free(lhs);
	kn_value_free(rhs);

	return ret;
}


KN_DECLARE_FUNCTION(list_delete_fn, 2, "X_LDELETE") {
	kn_value ran = kn_value_run(args[0]);
	kn_number index = kn_value_to_number(args[1]);
	kn_value deleted = list_delete(VALUE2LIST(ran), CORRECT_INDEX(index));

	kn_value_free(ran);

	return deleted;
}
#undef VALUE2LIST
#undef CORRECT_INDEX

kn_number _list_length(struct list *list) {
	return list->length;
}

const struct kn_custom_vtable list_vtable = {
	.free = (void (*)(void *)) list_free,
	.dump = (void (*)(void *)) list_dump,
	.run = (kn_value (*)(void *)) list_run,
	.to_number = (kn_number (*)(void *)) _list_length,
	.to_boolean = (kn_boolean (*) (void *)) list_is_empty,
	.to_string = (struct kn_string *(*)(void *)) list_to_string
};


kn_value parse_extension_list() {
	TRY_PARSE_FUNCTION("LPOP", list_pop_fn);
	TRY_PARSE_FUNCTION("LPUSH", list_push_fn);
	TRY_PARSE_FUNCTION("LGET", list_get_fn);
	TRY_PARSE_FUNCTION("LSET", list_set_fn);
	TRY_PARSE_FUNCTION("LINSERT", list_insert_fn);
	TRY_PARSE_FUNCTION("LDELETE", list_delete_fn);
	TRY_PARSE_FUNCTION("+", list_concat_fn);

	if (stream_starts_with_strip("["))
		return parse_list();
	if (stream_starts_with_strip("]")) {
		if (!depth--)
			die("unexpected `X]`");
		return KN_NULL;
	}

	return KN_UNDEFINED;
}
