#ifndef CLASS_H
#define CLASS_H

#include "../src/custom.h"
#include "function.h"
#include <stddef.h>

extern const struct kn_custom_vtable class_vtable, instance_vtable;

#define MAX_FIELDC 65535
#define MAX_METHODC 65535

struct class {
	const char *name;

	unsigned short fieldc, methodc;
	struct kn_variable **fields;

	struct function *constructor;
	struct function *to_number, *to_boolean, *to_string;
	struct function **methods;
};

struct instance {
	const struct class *class;
	kn_value fields[];
};

void free_class(struct class *);
void dump_class(const struct class *);

struct instance *new_instance(struct class *);
void free_instance(struct instance *);
void dump_instance(const struct instance *);

kn_value fetch_instance_field(const struct instance *, const char *);
struct function *fetch_method(const struct instance *, const char *);
void assign_instance_field(struct instance *, const char *, kn_value);
kn_value call_class_method(struct instance *, struct function_call *);

kn_value parse_extension_class(void);

#endif /* !CLASS_H */

