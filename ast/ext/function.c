#include "function.h"
#include "ext.h"
#include "../src/shared.h"
#include "../src/env.h"
#include <ctype.h>
#include <string.h>
#include <assert.h>

void free_function(struct function *function) {
	kn_value_free(function->body);
}

void free_function_call(struct function_call *fn_call) {
	kn_value_free(fn_call->func);

	for (size_t i = 0; i < fn_call->argc; ++i)
		kn_value_free(fn_call->args[i]);
}

static void unsupported_function(void *arg) {
	(void) arg;

	die("unsupported operation on function");
}

kn_value run_function_call(struct function_call *func_call) {
	size_t argc = func_call->argc;

	struct function *func = (struct function *)
		kn_value_as_custom(kn_value_run(func_call->func))->data;

	if (argc != func->paramc) {
		die("param mismatch (given %zu, expected %zu)",
			func_call->argc, func->paramc);
	}

	kn_value saved[argc];
	kn_value current[argc]; // so we can free it when done

	for (size_t i = 0; i < argc; ++i) {
		saved[i] = func->params[i]->value;
		current[i] = func->params[i]->value = kn_value_run(func_call->args[i]);
	}

	kn_value result = kn_value_run(func->body);

	for (size_t i = 0; i < argc; ++i) {
		func->params[i]->value = saved[i]; // restore old value when returning
		kn_value_free(current[i]); // free the value, as we ran it earlier.
	}

	kn_custom_free(container_of(func, struct kn_custom, data));

	return result;
}


const struct kn_custom_vtable function_call_vtable = {
	.free = (void (*)(void *)) free_function_call,
	.run = (kn_value (*)(void *)) run_function_call,
};

const struct kn_custom_vtable function_vtable = {
	.free = (void (*)(void *)) free_function,
	.to_number = (kn_number (*)(void *)) unsupported_function,
	.to_string = (struct kn_string *(*)(void *)) unsupported_function,
	.to_boolean = (kn_boolean (*)(void *)) unsupported_function,
};

kn_value parse_function_declaration(void) {
	struct kn_variable *params[MAX_ARGC];
	size_t paramc = 0;
	kn_value body;

	kn_parse_strip();

	if (!islower(kn_parse_peek()) && kn_parse_peek() != '_')
		die("function names must be variables.");

	struct kn_variable *name = kn_parse_variable();

	do {
		body = kn_parse_value();

		if (body == KN_UNDEFINED)
			die("missing a body for function '%s'", name->name);

		// if the body isn't a variable, then it's actually the body, and we are
		// done
		if (!kn_value_is_variable(body))
			break;

		// otherwise, the body is actually a variable
		params[paramc++] = kn_value_as_variable(body);
	} while (paramc < MAX_ARGC);

	struct kn_custom *custom = kn_custom_alloc(
		sizeof(struct function) + sizeof(struct kn_variable *[paramc]),
		&function_vtable
	);

	struct function *function = (struct function *) custom->data;
	function->body = body;
	function->paramc = paramc;
	memcpy(function->params, params, sizeof(struct kn_variable *[paramc]));

	kn_value result = kn_value_new_custom(custom);
	kn_variable_assign(name, result);

	return kn_value_clone(result);
}

static unsigned function_call_depth;

#define END_FUNCTION_CALL KN_CUSTOM_UNDEFINED(1)

static kn_value parse_function_call() {
	kn_value args[MAX_ARGC];
	size_t argc = 0;
	unsigned start_depth = function_call_depth++;

	kn_value func = kn_parse_value();
	if (func == KN_UNDEFINED) die("missing function for XCALL");

	kn_value parsed;

	do {
		parsed = kn_parse_value();

		switch (parsed) {
		case END_FUNCTION_CALL:
			assert(start_depth == function_call_depth);
			goto done;
		case KN_UNDEFINED:
			die("missing closing 'X)'");
		default:
			args[argc++] = parsed;
		}
	} while (argc < MAX_ARGC);

done:
	;
	struct kn_custom *custom = kn_custom_alloc(
		sizeof(struct function_call) + sizeof(kn_value [argc]),
		&function_call_vtable
	);

	struct function_call *function_call = (struct function_call *) custom->data;

	function_call->func = func;
	function_call->argc = argc;
	memcpy(function_call->args, args, sizeof(kn_value [argc]));

	return kn_value_new_custom(custom);
}

kn_value parse_extension_function() {
	if (stream_starts_with_strip("FUNC"))
		return parse_function_declaration();

	if (stream_starts_with_strip("CALL"))
		return parse_function_call();

	if (stream_starts_with_strip(")")/* || kn_parse_stream[-2] == ')'*/) {
		if (!function_call_depth--) die("stray 'X)' encountered");
		return END_FUNCTION_CALL;
	}

	return KN_UNDEFINED;
}
