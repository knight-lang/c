#ifndef FUNCTION_H
#define FUNCTION_H

#include "../src/custom.h"
#include <stddef.h>

extern const struct kn_custom_vtable function_vtable;
extern const struct kn_custom_vtable function_call_vtable;


#ifndef MAX_ARGC
# define MAX_ARGC 255
#endif


// user function.
struct function {
	size_t paramc;
	const char *name;
	kn_value body;
	struct kn_variable *params[];
};

struct function_call {
	size_t argc;
	kn_value func;
	kn_value args[];
};

void free_function(struct function *);
void free_function_call(struct function_call *);
kn_value run_function_call(struct function_call *);

kn_value parse_extension_function(void);

#endif /* !FUNCTION_H */
