#include "../src/parse.h"
#include "../src/shared.h"
#include "file.h"
#include "list.h"

#include <ctype.h>
#include <string.h>

/*
unsigned ext_depth;

kn_value parse_value(char skip) {
	char c;

top:
	// ie everything but `skip`.
	while (
		(c == '(' || c == ')'
			|| c == '{' || c == '}'
			|| c == '[' || c == ']'
			|| c == ':' || isspace(c = kn_parse_peek()))
		&& c != skip
	) kn_parse_advance();

	if (kn_parse_peek() == '#') {
		while (kn_parse_advance_peek() != '\n');
		goto top;
	}

	if (c == skip) {
		--ext_depth;
		kn_parse_advance();
		return KN_UNDEFINED;
	}

	return kn_parse_value();
}*/

kn_value kn_parse_extension_greeter(void);
kn_value kn_parse_extension_ufunc(void);

kn_value kn_parse_extension() {
	kn_value value;

	while (kn_parse_peek() == '_')
		kn_parse_advance();

	if ((value = kn_parse_extension_greeter()) != KN_UNDEFINED) return value;
	if ((value = kn_parse_extension_list()) != KN_UNDEFINED) return value;
	if ((value = kn_parse_extension_file()) != KN_UNDEFINED) return value;
	if ((value = kn_parse_extension_ufunc()) != KN_UNDEFINED) return value;

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
