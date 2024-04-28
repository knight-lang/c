#ifdef KN_FUZZING
struct _ignored;
#else
#include "knight.h" /* kn_startup, kn_play, kn_value_free, kn_shutdown */
#include "shared.h" /* kn_die, kn_heap_malloc, kn_heap_realloc */
#include "env.h"

#include <stdlib.h> /* free, NULL, size_t */
#include <stdio.h>  /* FILE, fopen, feof, fread, fclose, perror, EOF */
#include <string.h> /* strcmp, strerror */

#ifndef KN_RECKLESS
# include <errno.h> /* errno */
#endif /* !KN_RECKLESS */

/**
 * The amount of buckets that the `kn_env_map` will have.
 *
 * The greater the number, the fewer cache collisions, but the more memory used.
 **/
#ifndef KN_ENV_NBUCKETS
# define KN_ENV_NBUCKETS 65536
#endif /* !KN_ENV_NBUCKETS */

/**
 * The capacity of each bucket.
 *
 * Once this many variables are in a single bucket, the program will have to
 * reallocate those buckets.
 **/
#ifndef KN_ENV_CAPACITY
# define KN_ENV_CAPACITY 256
#endif /* !KN_ENV_CAPACITY */

#if KN_ENV_CAPACITY == 0
# error env capacity must be at least 1
#endif /* KN_ENV_CAPACITY == 0 */


static char *read_file(const char *filename) {
	FILE *file = fopen(filename, "r");

	if (file == NULL) {
		KN_MSVC_SUPPRESS(4996)
		kn_error("unable to read file '%s': %s", filename, strerror(errno));
	}

	size_t length = 0;
	size_t capacity = 2048;
	char *contents = kn_heap_malloc(capacity);

	while (!feof(file)) {
		size_t amntread = fread(&contents[length], 1, capacity - length, file);

		if (amntread == 0) {
			if (ferror(file)) {
				KN_MSVC_SUPPRESS(4996)
				kn_error("unable to read file '%s': %s", filename, strerror(errno));
			}
			break;
		}

		length += amntread;

		if (length == capacity) {
			capacity *= 2;
			contents = kn_heap_realloc(contents, capacity);
		}
	}

	if (fclose(file) == EOF) {
		KN_MSVC_SUPPRESS(4996)
		kn_error("couldn't close input file: %s", strerror(errno));
	}

	contents = kn_heap_realloc(contents, length + 1);
	contents[length] = '\0';
	return contents;
}

int main(int argc, char **argv) {
	char *str;

	if (argc != 3 || (!strcmp(argv[1], "-e") && !strcmp(argv[1], "-f")))
		goto usage;


	switch (argv[1][1]) {
	case 'e':
		str = argv[2];
		break;

	case 'f':
		str = read_file(argv[2]);
		break;

	default:
	usage:
		fprintf(stderr, "usage: %s (-e 'expr' | -f file)", argv[0]);
		return 1;
	}

	kn_startup();
	struct kn_env *env = kn_env_create(KN_ENV_CAPACITY, KN_ENV_NBUCKETS);

#ifdef KN_RECKLESS
	kn_play(env, str, strlen(str));
#else
	kn_value_free(kn_play(env, str, strlen(str)));
	kn_env_destroy(env);
	kn_shutdown();

	if (argv[1][1] == 'f')
		kn_heap_free(str);
#endif /* KN_RECKLESS */

	return 0;
}
#endif
