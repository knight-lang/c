#ifndef FUNCTION_H
#define FUNCTION_H

#include "../src/custom.h"
#include <stddef.h>

extern const struct kn_custom_vtable function_vtable;
extern const struct kn_custom_vtable function_call_vtable;

#ifndef MAX_PARAMS
# define MAX_PARAMS 255
#endif
#ifndef MAX_LOCALS
# define MAX_LOCALS 4095
#endif

// user function.
struct function {
	unsigned short nparams, nlocals;
	const char *name;
	kn_value body;
	struct kn_variable **locals, **params;
};

struct function_call {
	unsigned short nargs;
	kn_value func;
	kn_value args[];
};

void free_function(struct function *);
void free_function_call(struct function_call *);
kn_value run_function_call(struct function_call *);
kn_value run_function(struct function *, kn_value *);

kn_value parse_function_declaration(void);
kn_value parse_function_call(void);
kn_value parse_extension_function(void);

#endif /* !FUNCTION_H */
