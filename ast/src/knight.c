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

#ifdef KN_CUSTOM
extern void kn_extension_startup(void);
#endif /* KN_CUSTOM */

void kn_startup() {
	kn_function_startup();
	kn_env_startup();
#ifdef KN_CUSTOM
	kn_extension_startup();
#endif /* KN_CUSTOM */
}

void kn_shutdown() {
	kn_env_shutdown();
	kn_ast_cleanup();
	kn_string_cleanup();
}

kn_value kn_run(const char *stream) {
	kn_value parsed = kn_parse(stream);

#ifndef KN_RECKLESS
	if (parsed == KN_UNDEFINED)
		die("unable to parse stream");
#endif /* !KN_RECKLESS */

	kn_value ret = kn_value_run(parsed);
	kn_value_free(parsed);
	return ret;
}
