#include "function.h"
#include "ext.h"
#include "../src/shared.h"
#include "../src/env.h"
#include <ctype.h>
#include <string.h>
#include <assert.h>

void free_function(struct function *function) {
	// dont free name, as it came from a variable.
	kn_value_free(function->body);
	free(function->locals);
	free(function->params);
}

void free_function_call(struct function_call *fn_call) {
	kn_value_free(fn_call->func);

	for (unsigned short i = 0; i < fn_call->nargs; ++i)
		kn_value_free(fn_call->args[i]);

}

kn_value run_function(struct function *function, kn_value *args) {
	unsigned short nparams = function->nparams;
	unsigned short nlocals = function->nlocals;

	kn_value saved_params[nparams], current_params[nparams], saved_locals[nlocals];

	// save locals and params
	for (unsigned short i = 0; i < nparams; ++i) {
		saved_params[i] = function->params[i]->value;
		current_params[i] = function->params[i]->value = kn_value_run(args[i]);
	}

	for (unsigned short i = 0; i < nlocals; ++i) {
		saved_locals[i] = function->locals[i]->value;
		function->locals[i]->value = KN_NULL;
	}

	kn_value result = kn_value_run(function->body);

	for (size_t i = 0; i < nparams; ++i) {
		function->params[i]->value = saved_params[i]; // restore old value when returning
		if (current_params[i] != KN_UNDEFINED)
			kn_value_free(current_params[i]);  // free the value, as we ran it earlier.
	}

	for (unsigned short i = 0; i < nlocals; ++i) {
		if (saved_locals[i] != KN_UNDEFINED) kn_value_free(saved_locals[i]);
		function->locals[i]->value = saved_locals[i];
	}

	return result;

}

kn_value run_function_call(struct function_call *func_call) {
	struct function *function = VALUE2DATA(kn_value_run(func_call->func));

	if (func_call->nargs != function->nparams)
		die("param mismatch (given %hd, expected %hd)", func_call->nargs, function->nparams);

	kn_value result = run_function(function, func_call->args);

	kn_value_free(DATA2VALUE(function));

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
	struct kn_variable *params[MAX_PARAMS], *locals[MAX_LOCALS];
	struct function *function = ALLOC_DATA(sizeof(struct function), &function_vtable);

	function->nparams = 0;
	function->nlocals = 0;

	kn_parse_strip();
	if (!islower(kn_parse_peek()) && kn_parse_peek() != '_')
		die("function names must be variables.");

	struct kn_variable *name = kn_parse_variable();
	function->name = name->name; // no need to clone bc its a variable.

	bool is_params = true;
	kn_value body;

	while(true) {
		kn_parse_strip();
		if (stream_starts_with_strip("LOCAL")) is_params = false;

		body = kn_parse_value();

		if (body == KN_UNDEFINED)
			die("missing a body for function '%s'", name->name);

		// if the body isn't a variable, then it's actually the body, and we are
		// done
		if (!kn_value_is_variable(body))
			break;

		// otherwise, the body is actually a variable
		if (function->nparams >= MAX_PARAMS) die("too many parameters given");
		if (function->nlocals >= MAX_PARAMS) die("too many locals given");

		if (is_params)
			params[function->nparams++] = kn_value_as_variable(body);
		else
			locals[function->nlocals++] = kn_value_as_variable(body);
	}

	function->body = body;
	function->params = memdup(params, sizeof(struct kn_variable *[function->nparams]));
	function->locals = memdup(locals, sizeof(struct kn_variable *[function->nlocals]));

	kn_value result = DATA2VALUE(function);
	kn_variable_assign(name, result);

	return kn_value_clone(result);
}

static unsigned function_call_depth;

#define END_FUNCTION_CALL KN_CUSTOM_UNDEFINED(1)

kn_value parse_function_call() {
	kn_value args[MAX_PARAMS];
	unsigned short nargs = 0;
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
			args[nargs++] = parsed;
		}
	} while (nargs < MAX_PARAMS);

done:
	;
	struct function_call *function_call = ALLOC_DATA(
		sizeof(struct function_call) + sizeof(kn_value [nargs]),
		&function_call_vtable
	);

	function_call->func = func;
	function_call->nargs = nargs;
	memcpy(function_call->args, args, sizeof(kn_value [nargs]));

	return DATA2VALUE(function_call);
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
