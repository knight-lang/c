#include "../src/custom.h"
#include "../src/function.h"
#include "../src/string.h"
#include "../src/shared.h"
#include "../src/parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct greeter {
	char *name, *greeting;
	unsigned rc;
};

struct greeter *clone_greeter(struct greeter *greeter) {
	++greeter->rc;

	return greeter;
}

void free_greeter(struct greeter *greeter) {
	if (--greeter->rc) return;
	free(greeter->name);
	free(greeter->greeting);
	free(greeter);
}

void dump_greeter(struct greeter *greeter) {
	printf("Greeter(\"%s\", \"%s\")", greeter->greeting, greeter->name);
}

struct kn_string *greet_string(struct greeter *greeter) {
	struct kn_string *string = kn_string_alloc(
		strlen(greeter->name) + strlen(greeter->greeting) + 3
	);

	strcpy(kn_string_deref(string), greeter->greeting);
	strcat(kn_string_deref(string), ", ");
	strcat(kn_string_deref(string), greeter->name);
	strcat(kn_string_deref(string), "!");

	return string;
}

kn_value greet_run(struct greeter *greeter) {
	return kn_value_new_string(greet_string(greeter));
}

static struct kn_custom_vtable greeter_vtable = {
	.clone = (void *(*)(void *)) clone_greeter,
	.free = (void (*)(void *)) free_greeter,
	.dump = (void (*)(void *)) dump_greeter,
	.run = (kn_value (*)(void *)) greet_run,
	.to_number = NULL,
	.to_boolean = NULL,
	.to_string = (struct kn_string *(*) (void *)) greet_string
};

kn_value kn_parse_extension(const char **stream) {
	kn_value next;
	struct kn_string *string;

	if (**stream != 'G') die("unknown start '%c'", (*stream)[0]);
	do ++*stream; while('A' <= **stream && **stream <= 'Z' || **stream == '_');
	struct greeter *greeter = xmalloc(sizeof(struct greeter));

	next = kn_parse(stream);
	if (next == KN_UNDEFINED) die("missing greeting for 'XGET'");
	string = kn_value_to_string(next);
	greeter->greeting = strdup(kn_string_deref(string));
	kn_string_free(string);
	kn_value_free(next);

	next = kn_parse(stream);
	if (next == KN_UNDEFINED) die("missing name for 'XGET'");
	string = kn_value_to_string(next);
	greeter->name = strdup(kn_string_deref(string));
	kn_string_free(string);
	kn_value_free(next);
	
	return kn_value_new_custom(greeter, &greeter_vtable);
	// static unsigned depth;

	// const struct kn_function *function;

	// switch (*(*stream)++) {
	// case '[': {
	// 	unsigned current_depth = depth++;
	// 	struct kn_list *head = NULL, *curr;
	// 	kn_value parsed;

	// 	while ((parsed = kn_parse(stream)) != KN_UNDEFINED) {
	// 		if (head == NULL)
	// 			head = curr = kn_list_new(parsed);
	// 		else
	// 			curr = curr->next = kn_list_new(parsed);
	// 	}

	// 	if (current_depth != depth) 
	// 		die("missing closing 'X]'");

	// 	if (head == NULL)
	// 		head = &kn_list_empty;

	// 	return kn_value_new_custom(head, &kn_list_vtable);
	// }

	// case ']':
	// 	if (!depth--)
	// 		die("unexpected `X]`");
	// 	return KN_UNDEFINED;
	// case 'A': 
	// 	function = &kn_fn_car;
	// 	goto parse_function;
	// case 'D':
	// 	function = &kn_fn_cdr;
	// 	goto parse_function;
	// case 'C':
	// 	function = &kn_fn_cons;

	// parse_function: {
	// 	struct kn_ast *ast = kn_ast_alloc(function->arity);

	// 	ast->func = function;
	// 	ast->refcount = 1;

	// 	for (size_t i = 0; i < function->arity; ++i) {
	// 		if ((ast->args[i] = kn_parse(stream)) == KN_UNDEFINED) {
	// 			die("unable to parse argume?nt %d for function '%c'",
	// 				i, function->name);
	// 		}
	// 	}

	// 	return kn_value_new_ast(ast);
	// }

	// default:
		// die("unknown extension character '%c'", (*stream)[-1]);
	// }
}


// KN_FUNCTION_DECLARE(extension, 1, 'X') {
// 	struct greeter * = xmalloc(sizeof(struct kn_custom));
// 	struct greeter *data = xmalloc(sizeof(struct greeter));
// 	struct kn_string *name = kn_value_to_string(args[0]);

// 	data->name = strdup(kn_string_deref(name));
// 	kn_string_free(name);
// 	data->greeting = strdup("Hello");
// 	data->rc = 1;

// 	custom->data = data;
// 	custom->vtable = &hw_vtable;

// 	return kn_value_new_custom(custom);
// }
