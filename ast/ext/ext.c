#include "../src/parse.h"
#include "../src/shared.h"
#include "../src/custom.h"
#include "ext.h"
#include <ctype.h>
#include <string.h>

void unsupported_function(void *arg) {
	(void) arg;

	die("unsupported operation");
}

extern kn_value (*kn_parse_functions[256])(void);
extern kn_value parse_list(void);

struct list;
extern struct list *list_concat(struct list *, struct list *);


KN_DECLARE_FUNCTION(list_concat_fn_or_default, 2, "+") {
	kn_value lhs = kn_value_run(args[0]);
	if (kn_value_is_custom(lhs)) {
		kn_value rhs = kn_value_run(args[1]);
		kn_value ret = kn_value_new_custom(list_concat(
			(struct list *)(kn_value_as_custom(lhs)->data), 
			(struct list *)(kn_value_as_custom(rhs)->data)));

		kn_value_free(lhs);
		kn_value_free(rhs);
		return ret;
	} else {
		kn_value args2[2] = { lhs, args[1] };
		kn_value ret = kn_fn_add_function(args2);
		kn_value_free(lhs);
		return ret;
	}
}

kn_value parse_list_and_strip(void) {
	kn_parse_advance();
	return parse_list();
}

kn_value parse_list_concat_and_strip(void) {
	kn_parse_advance();

	return kn_value_new_ast(kn_parse_ast(&list_concat_fn_or_default));
}

void kn_extension_startup() {
	kn_parse_functions['['] = parse_list_and_strip;
	kn_parse_functions['+'] = parse_list_concat_and_strip;
}

kn_value parse_extension_class(void);
kn_value parse_extension_greeter(void);
kn_value parse_extension_list(void);
kn_value parse_extension_file(void);
kn_value parse_extension_function(void);

kn_value kn_parse_extension() {
	kn_value value;

	while (kn_parse_peek() == '_')
		kn_parse_advance();

	// helper for `IF`to make it look nicer
	if (stream_starts_with_strip("ELSE")) return kn_parse_value();

	if ((value = parse_extension_class()) != KN_UNDEFINED) return value;
	if ((value = parse_extension_greeter()) != KN_UNDEFINED) return value;
	if ((value = parse_extension_list()) != KN_UNDEFINED) return value;
	if ((value = parse_extension_file()) != KN_UNDEFINED) return value;
	if ((value = parse_extension_function()) != KN_UNDEFINED) return value;

	die("unknown extension character '%c'", *kn_parse_stream);
}

bool stream_starts_with(const char *str) {
	unsigned i = -1;

	do {
		++i;

		if (str[i] == '\0') // we've reached the end.
			return true;

		if (kn_parse_stream[i] == '\0') // stream is not over
			return false;
	} while (str[i] == kn_parse_stream[i]);

	return false; // not equal
}

void strip_keyword() {
	char c;

	while (isupper(c = kn_parse_peek()) || c == '_')
		kn_parse_advance();
}

bool stream_starts_with_strip(const char *str) {
	if (!stream_starts_with(str))
		return false;

	kn_parse_stream += strlen(str);
	return true;
}
