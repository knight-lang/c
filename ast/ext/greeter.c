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
};

void free_greeter(struct greeter *greeter) {
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

kn_value run_greeter(struct greeter *greeter) {
	return kn_value_new_string(greet_string(greeter));
}

static struct kn_custom_vtable greeter_vtable = {
	.free = (void (*)(void *)) free_greeter,
	.dump = (void (*)(void *)) dump_greeter,
	.run = (kn_value (*)(void *)) run_greeter,
	.to_number = NULL,
	.to_boolean = NULL,
	.to_string = (struct kn_string *(*) (void *)) greet_string
};

kn_value kn_parse_extension(const char **stream) {
	kn_value next;
	struct kn_string *string;

	if (**stream != 'G') die("unknown start '%c'", (*stream)[0]);
	do ++*stream; while(('A' <= **stream && **stream <= 'Z') || **stream == '_');
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
}