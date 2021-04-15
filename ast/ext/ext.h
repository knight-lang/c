#ifndef EXT_H
#define EXT_H

#include "../src/value.h"
#include "../src/parse.h"

kn_value kn_parse_extension(void);

// kn_value parse_value(char skip);
void strip_keyword(void);
bool stream_starts_with(const char *str);
bool stream_starts_with_strip(const char *str);

#define TRY_PARSE_FUNCTION(string, function) \
	if (stream_starts_with_strip(string)) \
		return kn_value_new_ast(kn_parse_ast(&kn_fn_##function));

#endif /* EXT_H */
