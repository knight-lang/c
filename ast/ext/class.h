#ifndef CLASS_H
#define CLASS_H

#include "../src/custom.h"
#include "function.h"
#include <stddef.h>

extern const struct kn_custom_vtable class_vtable, instance_vtable;

#define MAX_NFIELDS 65535
#define MAX_NMETHODS 65535
#define MAX_NSTATICS 65535

struct class {
	const char *name;

	unsigned short nfields, nmethods, nstatics;
	struct kn_variable **fields;

	struct function *constructor, *to_number, *to_boolean, *to_string;
	struct function **statics;
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

kn_value parse_extension_class(void);

#endif /* !CLASS_H */

