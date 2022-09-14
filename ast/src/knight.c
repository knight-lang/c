#include "knight.h"   /* prototypes, kn_value, kn_value_run, kn_value_free
                         KN_UNDEFINED */
#include "function.h" /* kn_function_startup */
#include "parse.h"    /* kn_parse */
#include "env.h"      /* kn_env_startup, kn_env_shutdown */
#include "ast.h"      /* kn_ast_cleanup */
#include "string.h"   /* kn_string_cleanup */

#ifndef KN_RECKLESS
#include "shared.h"   /* die */
#endif /* !KN_RECKLESS */

void kn_startup() {
	kn_function_startup();
	kn_env_startup();
}

void kn_shutdown() {
	kn_env_shutdown();
	kn_ast_cleanup();
	kn_string_cleanup();
}

kn_value kn_run(const char *stream) {
	kn_value parsed = kn_parse(stream);

	if (parsed == KN_UNDEFINED)
		kn_error("unable to parse stream");

	kn_value ret = kn_value_run(parsed);
	kn_value_free(parsed);
	return ret;
}
