#ifndef CLASS_H
#define CLASS_H

#include "../src/custom.h"
#include "ufunc.h"
#include <stddef.h>

extern const struct kn_custom_vtable class_vtable;

struct method {
	const char *name;
	struct ufunc func;
};

struct class {
	const char *name;

	size_t fieldc;
	const char **fields;

	size_t methodc;
	struct method *methods;
};

void class_free(struct class *);
void class_dump(const struct class *);

kn_value method_call(struct class *, struct method *);

// struct list {
// 	size_t length, capacity;
// 	kn_value *elements;
// 	bool idempotent;
// };

// kn_value list_pop(struct list *);
// void list_push(struct list *, kn_value);
// kn_value list_get(const struct list *, size_t);
// void list_set(struct list *, size_t, kn_value);
// void list_insert(struct list *, size_t, kn_value);
// kn_value list_delete(struct list *, size_t);

// bool list_is_empty(const struct list *);
// struct kn_custom *list_concat(struct list *, struct list *);

// kn_value list_run(struct list *);
// void list_free(struct list *);
// void list_dump(const struct list *);

// struct kn_string *list_to_string(struct list *);

kn_value parse_extension_klass(void);

#endif /* !CLASS_H */

