#ifndef LIST_H
#define LIST_H

#include "../src/value.h"
#include "../src/custom.h"
#include <stdbool.h>
#include <stdint.h>

extern const struct kn_custom_vtable list_vtable;

struct list {
	size_t length, capacity;
	kn_value *elements;
	bool idempotent;
};

kn_value list_pop(struct list *);
void list_push(struct list *, kn_value);
kn_value list_get(const struct list *, size_t);
void list_set(struct list *, size_t, kn_value);
void list_insert(struct list *, size_t, kn_value);
kn_value list_delete(struct list *, size_t);

bool list_is_empty(const struct list *);
struct kn_custom *list_concat(struct list *, struct list *);

kn_value list_run(struct list *);
void list_free(struct list *);
void list_dump(const struct list *);

struct kn_string *list_to_string(struct list *);

kn_value parse_extension_list(void);

#endif /* !LIST_H */

