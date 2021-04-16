#include "class.h"

/*
struct method {
	const char *name;
	struct ufunc func;
};

struct class {
	const char *name;

	size_t fieldc;
	const char **fields;

	size_t methodc;
	struct method *methods;
};
*/

void class_free(struct class *class) {
	for (size_t i = 0; i < class->fieldc; ++i)
		free(class->fields[i]);
	free(class->fields);

	for (size_t i = 0; i < class->methodc; ++i) {
		free(class->methods[i]->name);
		ufunc_free(class->methods[i]->func);
	}
}

void class_dump(const struct class *class) {
	printf("Class(%s)", class->name);
}
