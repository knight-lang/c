#include "knight.h" /* kn_startup, kn_play, kn_value_free, kn_shutdown */
#include "shared.h" /* die, xmalloc, xrealloc */

#include <stdlib.h> /* free, NULL, size_t */
#include <stdio.h>  /* FILE, fopen, feof, fread, fclose, perror, EOF */
#include <string.h> /* strcmp, strerror */

#ifndef KN_RECKLESS
# include <errno.h> /* errno */
#endif /* !KN_RECKLESS */

static char *read_file(const char *filename) {
	FILE *file = fopen(filename, "r");

	if (file == NULL)
		kn_error("unable to read file '%s': %s", filename, strerror(errno));

	size_t length = 0;
	size_t capacity = 2048;
	char *contents = xmalloc(capacity);

	while (!feof(file)) {
		size_t amntread = fread(&contents[length], 1, capacity - length, file);

		if (amntread == 0) {
			if (ferror(file))
				kn_error("unable to read file '%s': %s'", filename, strerror(errno));
			break;
		}

		length += amntread;

		if (length == capacity) {
			capacity *= 2;
			contents = xrealloc(contents, capacity);
		}
	}

	if (fclose(file) == EOF)
		kn_error("couldn't close input file: %s", strerror(errno));

	contents = xrealloc(contents, length + 1);
	contents[length] = '\0';
	return contents;
}

static void usage(char *program) {
	die("usage: %s (-e 'expr' | -f file)", program);
}

int main(int argc, char **argv) {
	char *str;

	if (argc != 3 || (!strcmp(argv[1], "-e") && !strcmp(argv[1], "-f")))
		usage(argv[0]);

	switch (argv[1][1]) {
	case 'e':
		str = argv[2];
		break;

	case 'f':
		str = read_file(argv[2]);
		break;

	default:
		usage(argv[0]);
	}

	kn_startup();

#ifdef KN_RECKLESS
	kn_play(str);
#else
	kn_value_free(kn_play(str));
	kn_shutdown();

	if (argv[1][1] == 'f')
		free(str);
#endif /* KN_RECKLESS */

	return 0;
}
