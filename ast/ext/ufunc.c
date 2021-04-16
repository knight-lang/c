#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "ufunc.h"
#include "ext.h"
#include "../src/custom.h"
#include "../src/shared.h"
#include "../src/function.h"
#include "../src/string.h"
#include "../src/parse.h"
#include "../src/ast.h"

#include "../src/custom.h"
#include "../src/function.h"
#include "../src/string.h"
#include "../src/shared.h"
#include "../src/parse.h"
#include "../src/env.h"
#include <stdio.h>
#include <stdlib.h>
#include "ext.h"
#include <string.h>

#define UFUNC(data) ((struct ufunc *) (data))
#define UFUNC_CALL(data) ((struct ufunc_call *) (data))

struct ufunc_call {
	unsigned char argc;
	kn_value func;
	kn_value args[];
};

static void free_ufunc(void *data) {
	kn_value_free(UFUNC(data)->body);
	// no need to free the variables
}

static void dump_ufunc(void *data) {
	printf("UFunc([");

	for (size_t i = 0; i < UFUNC(data)->paramc; ++i) {
		if (i != 0)
			printf(", ");

		printf("%s", UFUNC(data)->params[i]->name);
	}

	printf("], ");
	kn_value_dump(UFUNC(data)->body);
	printf(")");
}

static void unsupported_function(void *arg) {
	(void) arg;

	die("unsupported operation on ufunc");
}


static void free_ufunc_call(void *data) {
	kn_value_free(UFUNC_CALL(data)->func);

	for (size_t i = 0; i < UFUNC_CALL(data)->argc; ++i)
		kn_value_free(UFUNC_CALL(data)->args[i]);
}

static kn_value run_ufunc_call(void *data) {
	struct ufunc_call *ufunc_call = UFUNC_CALL(data);
	kn_value ran = kn_value_run(ufunc_call->func);
	struct ufunc *ufunc = UFUNC(kn_value_as_custom(ran)->data);

	kn_value prev_vals[ufunc->paramc];
	kn_value curr_vals[ufunc->paramc];

	if (ufunc->paramc != ufunc_call->argc) {
		die("param mismatch (given %d, expected %d)",
			ufunc_call->argc, ufunc->paramc);
	}

	for (unsigned char i = 0; i < ufunc->paramc; ++i) {
		prev_vals[i] = ufunc->params[i]->value;
		ufunc->params[i]->value =
			curr_vals[i] = kn_value_run(ufunc_call->args[i]);
	}

	kn_value result = kn_value_run(ufunc->body);

	for (unsigned char i = 0; i < ufunc->paramc; ++i) {
		ufunc->params[i]->value = prev_vals[i];
		kn_value_free(curr_vals[i]);
	}

	kn_value_free(ran);

	return result;
}

static struct kn_custom_vtable ufunc_call_vtable = {
	.free = (void (*)(void *)) free_ufunc_call,
	.run = (kn_value (*)(void *)) run_ufunc_call,
};

const struct kn_custom_vtable ufunc_vtable = {
	.free = (void (*)(void *)) free_ufunc,
	.dump = (void (*)(void *)) dump_ufunc,
	.to_number = (kn_number (*)(void *)) unsupported_function,
	.to_string = (struct kn_string *(*)(void *)) unsupported_function,
	.to_boolean = (kn_boolean (*)(void *)) unsupported_function,
};

static kn_value parse_ufunc_declaration() {
	unsigned char paramc = 0;
	struct kn_variable *params[MAXARGC];
	kn_value body;

	do {
		body = kn_parse_value();

		if (body == KN_UNDEFINED) die("no body encountered!");
		if (!kn_value_is_variable(body)) break;

		params[paramc++] = kn_value_as_variable(body);
	} while (paramc < MAXARGC);

	struct kn_custom *custom = kn_custom_alloc(
		sizeof(struct ufunc) + sizeof(struct kn_variable *[paramc]),
		&ufunc_vtable
	);

	memcpy(
		UFUNC(custom->data)->params,
		params,
		sizeof(struct kn_variable *[paramc])
	);

	UFUNC(custom->data)->body = body;
	UFUNC(custom->data)->paramc = paramc;

	return kn_value_new_custom(custom);
}

static unsigned depth;

static kn_value parse_ufunc_call() {
	kn_value args[MAXARGC];
	unsigned char argc = 0;
	unsigned current_depth = depth++;

	kn_value func = kn_parse_value();
	if (func == KN_UNDEFINED) die("missing function for 'X('");

	kn_value parsed;

	do {
		parsed = kn_parse_value();

		if (current_depth == depth) break;
		if (parsed == KN_UNDEFINED) die("missing closing 'X)'");
		args[argc++] = parsed;
	} while (argc < MAXARGC);

	struct kn_custom *custom = kn_custom_alloc(
		sizeof(struct ufunc_call) + sizeof(kn_value[argc]),
		&ufunc_call_vtable
	);

	UFUNC_CALL(custom->data)->func = func;
	UFUNC_CALL(custom->data)->argc = argc;
	memcpy(UFUNC_CALL(custom->data)->args, args, sizeof(kn_value[argc]));

	return kn_value_new_custom(custom);
}

kn_value parse_extension_ufunc() {
	if (stream_starts_with_strip("FUNC"))
		return parse_ufunc_declaration();

	if (stream_starts_with_strip("(") || stream_starts_with_strip("CALL"))
		return parse_ufunc_call();

	if (stream_starts_with_strip(")")/* || kn_parse_stream[-2] == ')'*/) {
		if (!depth--) die("stray 'X)' encountered");
		return KN_NULL; // doesn't matter what we return, as we check depth.
	}

	return KN_UNDEFINED;
}
