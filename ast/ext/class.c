#include "class.h"
#include "ext.h"
#include "list.h"
#include "../src/custom.h"
#include "../src/function.h"
#include "../src/env.h"
#include "../src/string.h"
#include "../src/parse.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define CLASS(data) ((struct class *) data)
#define INSTANCE(data) ((struct instance *) data)

void free_class(struct class *class) {
	unsigned short i;
	// note we don't own the name, so dont free it

	if (class->constructor) free_function(class->constructor);
	if (class->to_string) free_function(class->to_string);
	if (class->to_boolean) free_function(class->to_boolean);
	if (class->to_number) free_function(class->to_number);

	for (i = 0; i < class->nfields; ++i) free(class->fields[i]);
	free(class->fields);

	for (i = 0; i < class->nparents; ++i)
		if (class->ran) free_class(class->parents[i]);
		else kn_value_free(class->uneval_parents[i]);
	free(class->parents);

	for (i = 0; i < class->nmethods; ++i) free_function(class->methods[i]);
	free(class->methods);

	for (i = 0; i < class->nstatics; ++i) free_function(class->statics[i]);
	free(class->statics);
}

void dump_class(const struct class *class) {
	printf("Class(%s)", class->name);
}

void *run_class(struct class *class) {
	if (!class->ran) {
		class->ran = 1;
		struct class **parents = malloc(sizeof(struct class *[class->nparents]));
		for (unsigned short i = 0; i < class->nparents; ++i)
			parents[i] = VALUE2DATA(kn_value_run(class->uneval_parents[i]));
		free(class->uneval_parents);
		class->parents = parents;
	}

	return kn_value_clone(DATA2VALUE(class));
}

struct kn_string *class_to_string(struct class *class) {
	size_t namelen = strlen(class->name);

	unsigned long hash = kn_hash("Class(", 6);
	hash = kn_hash_acc(class->name, namelen, hash);
	hash = kn_hash_acc(")", 1, hash);

	struct kn_string *string = kn_string_cache_lookup(hash, 7 + namelen);
	if (string != NULL) return kn_string_clone(string);

	string = kn_string_alloc(7 + namelen);
	sprintf(kn_string_deref(string), "Class(%s)", class->name);
	kn_string_cache(string);

	return string;
}

void free_instance(struct instance *instance) {
	for (unsigned short i = 0; i < instance->class->nfields; ++i)
		kn_value_free(instance->fields[i]);

	// release our reference to the class
	kn_value_free(DATA2VALUE(instance->class));
}

void dump_instance(const struct instance *instance) {
	printf("Instance(");
	dump_class(instance->class);

	for (unsigned short i = 0; i < instance->class->nfields; ++i) {
		printf(", %s=", instance->class->fields[i]->name);
		kn_value_dump(instance->fields[i]);
	}

	printf(")");
}

static kn_value run_method(
	struct instance *instance,
	struct function *function,
	kn_value *args
) {
	static struct kn_variable *this;
	if (!this) this = kn_env_fetch("this", 4);

	kn_value old_this = this->value;
	this->value = DATA2VALUE(instance);
	kn_value result = run_function(function, args);
	this->value = old_this;

	return result;
}

static kn_value NOARGS;

static struct kn_string *instance_to_string(struct instance *instance) {
	struct function *function = instance->class->to_string;
	if (!function)
		die("cannot call 'to_string' on an '%s'", instance->class->name);

	assert(function->nparams == 0);

	return kn_value_as_string(run_method(instance, function, &NOARGS));
}

static kn_number instance_to_number(struct instance *instance) {
	struct function *function = instance->class->to_number;
	if (!function)
		die("cannot call 'to_number' on an '%s'", instance->class->name);

	assert(function->nparams == 0);

	return kn_value_as_number(run_method(instance, function, &NOARGS));
}

static kn_boolean instance_to_boolean(struct instance *instance) {
	struct function *function = instance->class->to_boolean;
	if (!function)
		die("cannot call 'to_boolean' on an '%s'", instance->class->name);

	assert(function->nparams == 0);

	return kn_value_as_boolean(run_method(instance, function, &NOARGS));
}

static kn_value *find_field(struct instance *instance, const char *field) {
	for (unsigned short i = 0; i < instance->class->nfields; ++i)
		if (!strcmp(instance->class->fields[i]->name, field))
			return &instance->fields[i];

	die("unknown field '%s' for instance of '%s'", field, instance->class->name);
}

kn_value fetch_instance_field(const struct instance *instance, const char *field) {
	return kn_value_clone(*find_field((struct instance *) instance, field));
}

struct function *fetch_method(const struct class *class, const char *name) {
	struct function *method;
	unsigned short i;
	for (i = 0; i < class->nmethods; ++i)
		if (!strcmp((method = class->methods[i])->name, name))
			return method;

	if (!strcmp(name, "to_string") && (method = class->to_string)) return method;
	if (!strcmp(name, "to_number") && (method = class->to_number)) return method;
	if (!strcmp(name, "to_boolean") && (method = class->to_boolean)) return method;
	if (!strcmp(name, "constructor") && (method = class->constructor)) return method;

	for (i = 0; i < class->nparents; ++i) {
		printf("here\n");
		if ((method = fetch_method(class->parents[i], name))) return method;
	}

	return NULL;
}

struct function *fetch_static(const struct class *class, const char *name) {
	struct function *static_fn;
	unsigned short i;

	for (i = 0; i < class->nstatics; ++i)
		if (!strcmp((static_fn = class->statics[i])->name, name))
			return static_fn;

	for (i = 0; i < class->nparents; ++i)
		if ((static_fn = fetch_static(class->parents[i], name))) return static_fn;

	return NULL;
}

void assign_instance_field(struct instance *instance, const char *field, kn_value value) {
	kn_value *field_value = find_field(instance, field);

	kn_value_free(*field_value);

	*field_value = value;
}

KN_DECLARE_FUNCTION(instance_field_assign_fn, 3, "X.=") {
	struct instance *instance = VALUE2DATA(kn_value_run(args[0]));
	struct kn_variable *field = kn_value_as_variable(args[1]);
	kn_value value = kn_value_run(args[2]);

	assign_instance_field(instance, field->name, value);

	kn_value_free(DATA2VALUE(instance));

	return kn_value_clone(value);
}

KN_DECLARE_FUNCTION(instance_field_fetch_fn, 2, "X.") {
	struct instance *instance = VALUE2DATA(kn_value_run(args[0]));
	struct kn_variable *field = kn_value_as_variable(args[1]);

	kn_value value = fetch_instance_field(instance, field->name);

	kn_value_free(DATA2VALUE(instance));	

	return kn_value_clone(value);
}


KN_DECLARE_FUNCTION(instance_call_method_fn, 3, "X_CALLM") {
	struct instance *instance = VALUE2DATA(kn_value_run(args[0]));
	struct kn_variable *method_name = kn_value_as_variable(args[1]);
	struct list *params = VALUE2DATA(kn_value_run(args[2]));

	struct function *method = fetch_method(instance->class, method_name->name);
	if (!method)
		die("unknown method '%s' for instance of '%s'", method_name->name, instance->class->name);
	kn_value value = run_method(instance, method, params->elements);

	kn_value_free(DATA2VALUE(instance));
	list_free(params);

	return value;
}

KN_DECLARE_FUNCTION(instance_call_static_fn, 3, "X_CALLS") {
	struct class *class = VALUE2DATA(kn_value_run(args[0]));
	struct kn_variable *static_name = kn_value_as_variable(args[1]);
	struct list *params = VALUE2DATA(kn_value_run(args[2]));

	struct function *static_fn = fetch_static(class, static_name->name);
	if (!static_fn)
		die("unknown static_fn '%s' for instance of '%s'", static_name->name, class->name);

	kn_value value = run_function(static_fn, params->elements);

	kn_value_free(DATA2VALUE(class));
	list_free(params);

	return value;
}

KN_DECLARE_FUNCTION(instance_new_fn, 2, "X_NEW") {
	struct class *class = VALUE2DATA(kn_value_run(args[0]));
	struct list *params = VALUE2DATA(kn_value_run(args[1]));

	size_t expected_argc = class->constructor ? class->constructor->nparams : 0;
	if (params->length != expected_argc)
		die("constructor arg mismatch (expected %zu, got %zu)", expected_argc, params->length);

	struct instance *instance = ALLOC_DATA(
		sizeof(struct instance) + sizeof(kn_value[class->nfields]),
		&instance_vtable
	);

	// we need to clone it, as we have a reference to it.
	(void) kn_value_clone(DATA2VALUE(class));
	instance->class = class;

	for (unsigned short i = 0; i < class->nfields; ++i)
		instance->fields[i] = KN_NULL;

	if (instance->class->constructor)
		run_method(instance, instance->class->constructor, params->elements);
	list_free(params);

	return DATA2VALUE(instance);
}

KN_DECLARE_FUNCTION(instance_class_of, 1, "X_CLASSOF") {
	struct instance *instance = VALUE2DATA(kn_value_run(args[0]));
	struct class *class = instance->class;

	kn_value ret = kn_value_clone(DATA2VALUE(class));

	kn_value_free(DATA2VALUE(instance));

	return ret;
}

const struct kn_custom_vtable class_vtable = {
	.free = (void (*)(void *)) free_class,
	.dump = (void (*)(void *)) dump_class,
	.run = (void *(*)(void *)) run_class,
	.to_string = (struct kn_string *(*)(void *)) class_to_string,
	.to_number = (kn_number (*)(void *)) unsupported_function,
	.to_boolean = (kn_boolean (*)(void *)) unsupported_function
};

const struct kn_custom_vtable instance_vtable = {
	.free = (void (*)(void *)) free_instance,
	.dump = (void (*)(void *)) dump_instance,
	.to_string = (struct kn_string *(*)(void *)) instance_to_string,
	.to_number = (kn_number (*)(void *)) instance_to_number,
	.to_boolean = (kn_boolean (*)(void *)) instance_to_boolean
};

static kn_value parse_class() {
	static struct kn_variable *fields[MAX_NFIELDS];
	static struct function *methods[MAX_NMETHODS];
	static struct function *statics[MAX_NSTATICS];
	static struct function *parents[MAX_NPARENTS];

	kn_parse_strip();
	struct kn_variable *classname = kn_parse_variable();

	struct class *class = ALLOC_DATA(sizeof(struct class), &class_vtable);

	class->name = classname->name;
	class->nfields = 0;
	class->nparents = 0;
	class->nmethods = 0;
	class->nstatics = 0;
	class->constructor = NULL;
	class->to_string = NULL;
	class->to_number = NULL;
	class->to_boolean = NULL;
	class->ran = false;

	while (1) {
		kn_parse_strip();

		if (class->nfields != MAX_NFIELDS && stream_starts_with_strip("FIELD")) {
			do {
				kn_parse_strip();
				fields[class->nfields++] = kn_parse_variable();
				kn_parse_strip();
			} while (islower(kn_parse_peek()) || kn_parse_peek() == '_');
		} else if (class->nparents != MAX_NPARENTS && stream_starts_with_strip("PARENTS")) {
			do {
				kn_parse_strip();
				parents[class->nparents++] = kn_parse_variable();
				kn_parse_strip();
			} while (islower(kn_parse_peek()) || kn_parse_peek() == '_');
		} else if (class->nfields != MAX_NSTATICS && stream_starts_with_strip("STATIC METHOD")) {
			statics[class->nstatics++] = VALUE2DATA(parse_function_declaration());
		} else if (class->nmethods != MAX_NMETHODS && stream_starts_with_strip("METHOD")) {
			struct function *function = VALUE2DATA(parse_function_declaration());
			struct function **dst;

			// if they're assigned more than once, memory leak
			if (!strcmp(function->name, "constructor")) dst = &class->constructor;
			else if (!strcmp(function->name, "to_string")) dst = &class->to_string;
			else if (!strcmp(function->name, "to_boolean")) dst = &class->to_boolean;
			else if (!strcmp(function->name, "to_number")) dst = &class->to_number;
			else {
				methods[class->nmethods] = NULL;
				dst = &methods[class->nmethods++];
			}

			if (*dst != NULL) free_function(*dst);
			*dst = function;
		} else {
			break;
		}
	}

	class->fields = memdup(fields, sizeof(struct kn_variable *[class->nfields]));
	class->methods = memdup(methods, sizeof(struct function *[class->nmethods]));
	class->statics = memdup(statics, sizeof(struct function *[class->nstatics]));
	class->parents = memdup(parents, sizeof(kn_value [class->nparents]));

	kn_variable_assign(classname, kn_value_clone(DATA2VALUE(class)));

	// todo: this cloen seems out of place?
	return kn_value_clone(DATA2VALUE(class));
}

kn_value parse_extension_class() {
	if (stream_starts_with_strip("CLASS")) return parse_class();
	TRY_PARSE_FUNCTION(".=", instance_field_assign_fn);
	TRY_PARSE_FUNCTION(".", instance_field_fetch_fn);
	TRY_PARSE_FUNCTION("NEW", instance_new_fn);
	TRY_PARSE_FUNCTION("CALLM", instance_call_method_fn);
	TRY_PARSE_FUNCTION("CALLS", instance_call_static_fn);
	TRY_PARSE_FUNCTION("CLASSOF", instance_class_of);
	return KN_UNDEFINED;
}

