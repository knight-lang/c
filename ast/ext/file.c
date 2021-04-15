#include "file.h"
#include "ext.h"
#include "../src/shared.h"
#include "../src/function.h"
#include "../src/parse.h"
#include "../src/string.h"
#include <string.h> /* strcmp, strerror */
#include <errno.h>

static char *read_file(const char *filename, size_t *length) {
	FILE *file = fopen(filename, "r");

	if (file == NULL)
		die("unable to read file '%s': %s", filename, strerror(errno));

	*length = 0;
	size_t capacity = 2048;
	char *contents = xmalloc(capacity);

	while (!feof(file)) {
		size_t amnt = fread(&contents[*length], 1, capacity - *length, file);

		if (amnt == 0) {
			if (!feof(stdin)) {
				die("unable to line in file '%s': %s'", filename,
					strerror(errno));
			}
			break;
		}

		*length += amnt;

		if (*length == capacity) {
			capacity *= 2;
			contents = xrealloc(contents, capacity);
		}
	}

	if (fclose(file) == EOF)
		perror("couldn't close input file");

	contents = xrealloc(contents, *length + 1);
	contents[*length] = '\0';
	return contents;
}


KN_FUNCTION_DECLARE(xread, 1, "X_READ") {
	struct kn_string *string = kn_value_to_string(args[0]);
	size_t length;

	printf("%s\n", kn_string_deref(string));
	char *contents = read_file(kn_string_deref(string), &length);
	struct kn_string *result = kn_string_new_owned(contents, length);

	kn_string_free(string);

	return kn_value_new_string(result);
}

KN_FUNCTION_DECLARE(xappend, 2, "X_APPEND") {
	(void) args;
	die("todo");
}

kn_value kn_parse_extension_file(void) {
	if (stream_starts_with_strip("READ"))
		return kn_value_new_ast(kn_parse_ast(&kn_fn_xread));

	if (stream_starts_with_strip("APPEND"))
		return kn_value_new_ast(kn_parse_ast(&kn_fn_xappend));
		
	return KN_UNDEFINED;
}
