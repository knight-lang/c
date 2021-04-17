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

#define CLASS(data) ((struct class *) data)
#define INSTANCE(data) ((struct instance *) data)

void free_class(struct class *class) {
	// we don't own the name
	dump_class(class);
	printf("\n");
	if (class->to_string) free_function(class->to_string);
	if (class->to_boolean) free_function(class->to_boolean);
	if (class->to_number) free_function(class->to_number);

	for (unsigned short i = 0; i <class->fieldc; ++i)
		free(class->fields[i]);

	for (unsigned short i = 0; i < class->methodc; ++i)
		free_function(class->methods[i]);
}

void dump_class(const struct class *class) {
	printf("Class(%s)", class->name);
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

// struct instance *new_instance(struct class *class) {
// 	struct instance *instance = xmalloc(
// 		sizeof(struct instance) + sizeof(kn_value [class->fieldc])
// 	);

// 	// we need to clone it, as we have a reference to it.
// 	kn_value_clone(DATA2VALUE(class));

// 	instance->class = class;

// 	for (unsigned short i = 0; i < class->fieldc; ++i)
// 		instance->fields[i] = KN_NULL;

// 	return instance;
// }

void free_instance(struct instance *instance) {
	for (unsigned short i = 0; i < instance->class->fieldc; ++i)
		kn_value_free(instance->fields[i]);

	// release our reference to the class
	kn_value_free(DATA2VALUE(instance->class));
}

void dump_instance(const struct instance *instance) {
	printf("Instance(");
	dump_class(instance->class);

	for (unsigned short i = 0; i < instance->class->fieldc; ++i) {
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

	assert(function->paramc == 0);

	return kn_value_as_string(run_method(instance, function, &NOARGS));
}

static kn_number instance_to_number(struct instance *instance) {
	struct function *function = instance->class->to_number;
	if (!function)
		die("cannot call 'to_number' on an '%s'", instance->class->name);

	assert(function->paramc == 0);

	return kn_value_as_number(run_method(instance, function, &NOARGS));
}

static kn_boolean instance_to_boolean(struct instance *instance) {
	struct function *function = instance->class->to_boolean;
	if (!function)
		die("cannot call 'to_boolean' on an '%s'", instance->class->name);

	assert(function->paramc == 0);

	return kn_value_as_boolean(run_method(instance, function, &NOARGS));
}

static kn_value *find_field(struct instance *instance, const char *field) {
	for (unsigned short i = 0; i < instance->class->fieldc; ++i)
		if (!strcmp(instance->class->fields[i]->name, field))
			return &instance->fields[i];

	die("unknown field '%s' for instance of '%s'", field, instance->class->name);
}

kn_value fetch_instance_field(const struct instance *instance, const char *field) {
	return kn_value_clone(*find_field((struct instance *) instance, field));
}

struct function *fetch_method(const struct instance *instance, const char *name) {
	struct function *method;
	for (unsigned short i = 0; i < instance->class->methodc; ++i)
		if (!strcmp((method = instance->class->methods[i])->name, name))
			return method;

	if (!strcmp(name, "to_string") && (method = instance->class->to_string)) return method;
	if (!strcmp(name, "to_number") && (method = instance->class->to_number)) return method;
	if (!strcmp(name, "to_boolean") && (method = instance->class->to_boolean)) return method;
	if (!strcmp(name, "constructor") && (method = instance->class->constructor)) return method;
	die("unknown method '%s' for instance of '%s'", name, instance->class->name);
}

void assign_instance_field(struct instance *instance, const char *field, kn_value value) {
	kn_value *field_value = find_field(instance, field);

	kn_value_free(*field_value);

	*field_value = value;
}

kn_value call_class_method(struct instance *instance, struct function_call *call) {
	static struct kn_variable *this;
	if (!this) this = kn_env_fetch("this", 4);

	kn_value old_this = this->value;
	this->value = DATA2VALUE(instance);

	kn_value ret = run_function_call(call);

	this->value = old_this;

	return ret;
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


KN_DECLARE_FUNCTION(instance_call_method_fn, 3, "X_CALL_METHOD") {
	struct instance *instance = VALUE2DATA(kn_value_run(args[0]));
	struct kn_variable *method_name = kn_value_as_variable(args[1]);
	struct list *params = VALUE2DATA(kn_value_run(args[2]));

	struct function *method = fetch_method(instance, method_name->name);
	kn_value value = run_method(instance, method, params->elements);

	kn_value_free(DATA2VALUE(instance));
	list_free(params);

	return value;
}

KN_DECLARE_FUNCTION(instance_new_fn, 2, "X_NEW") {
	struct class *class = VALUE2DATA(kn_value_run(args[0]));
	struct list *params = VALUE2DATA(kn_value_run(args[1]));

	size_t expected_argc = class->constructor ? class->constructor->paramc : 0;
	if (params->length != expected_argc)
		die("constructor arg mismatch (expected %zu, got %zu)", expected_argc, params->length);

	struct instance *instance = ALLOC_DATA(
		sizeof(struct instance) + sizeof(kn_value[class->fieldc]),
		&instance_vtable
	);

	// we need to clone it, as we have a reference to it.
	(void) kn_value_clone(DATA2VALUE(class));
	instance->class = class;

	for (unsigned short i = 0; i < class->fieldc; ++i)
		instance->fields[i] = KN_NULL;

	if (instance->class->constructor)
		run_method(instance, instance->class->constructor, params->elements);
	list_free(params);

	return DATA2VALUE(instance);
}

KN_DECLARE_FUNCTION(instance_is_instance_fn, 2, "X_IS_INSTANCE") {
	struct instance *instance = VALUE2DATA(kn_value_run(args[0]));
	struct class *class = VALUE2DATA(kn_value_run(args[1]));

	kn_value isinstance = kn_value_new_boolean(instance->class == class);

	kn_value_free(DATA2VALUE(instance));
	kn_value_free(DATA2VALUE(class));

	return isinstance;
}

const struct kn_custom_vtable class_vtable = {
	.free = (void (*)(void *)) free_class,
	.dump = (void (*)(void *)) dump_class,
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
	static struct kn_variable *fields[MAX_FIELDC];
	static struct function *methods[MAX_METHODC];

	kn_parse_strip();
	struct kn_variable *classname = kn_parse_variable();

	struct class *class = ALLOC_DATA(sizeof(struct class), &class_vtable);

	class->name = classname->name;
	class->fieldc = 0;
	class->methodc = 0;
	class->constructor = NULL;
	class->to_string = NULL;
	class->to_number = NULL;
	class->to_boolean = NULL;

	while (1) {
		kn_parse_strip();

		if (class->fieldc != MAX_FIELDC && stream_starts_with_strip("FIELD")) {
			kn_parse_strip();
			fields[class->fieldc++] = kn_parse_variable();
		} else if (class->methodc != MAX_METHODC && stream_starts_with_strip("METHOD")) {
			struct function *function = VALUE2DATA(parse_function_declaration());
			struct function **dst;

			// if they're assigned more than once, memory leak
			if (!strcmp(function->name, "constructor"))
				dst = &class->constructor;
			else if (!strcmp(function->name, "to_string"))
				dst = &class->to_string;
			else if (!strcmp(function->name, "to_boolean"))
				dst = &class->to_boolean;
			else if (!strcmp(function->name, "to_number"))
				dst = &class->to_number;
			else {
				methods[class->methodc] = NULL;
				dst = &methods[class->methodc++];
			}

			if (*dst != NULL) free_function(*dst);
			*dst = function;
		} else {
			break;
		}
	}

	class->fields = memdup(fields, sizeof(struct kn_variable *[class->fieldc]));
	class->methods = memdup(methods, sizeof(struct function *[class->methodc]));

	kn_variable_assign(classname, kn_value_clone(DATA2VALUE(class)));

	// todo: this cloen seems out of place?
	return kn_value_clone(DATA2VALUE(class));
}

kn_value parse_extension_class() {
	if (stream_starts_with_strip("CLASS")) return parse_class();
	TRY_PARSE_FUNCTION(".=", instance_field_assign_fn);
	TRY_PARSE_FUNCTION(".", instance_field_fetch_fn);
	TRY_PARSE_FUNCTION("NEW", instance_new_fn);
	TRY_PARSE_FUNCTION("CALL_METHOD", instance_call_method_fn);
	TRY_PARSE_FUNCTION("IS_INSTANCE", instance_is_instance_fn);
	return KN_UNDEFINED;
}

