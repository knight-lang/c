#include "../src/parse.h"
#include "../src/shared.h"
#include "ext.h"
#include <ctype.h>
#include <string.h>

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
