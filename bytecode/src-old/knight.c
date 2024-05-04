#include "knight.h"
#include "parse.h"
#include "function.h"
#include "ast.h"
#include "string.h"

#ifndef KN_RECKLESS
#include "shared.h"
#endif /* !KN_RECKLESS */

void kn_startup(void) {
	kn_function_startup();

#ifdef KN_USE_GC
	kn_gc_init(1000000); // todo: variable size
#endif /* KN_USE_GC */
}

void kn_shutdown(void) {
	kn_ast_cleanup();
	kn_string_cleanup();

#ifdef KN_USE_GC
	kn_gc_teardown();
#endif /* KN_USE_GC */
}

#ifdef KN_FUZZING
jmp_buf kn_play_start;
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	kn_startup();
	struct kn_env *env = kn_env_create();

	kn_value value = kn_play(env, (const char *) data, size);

#ifdef KN_USE_REFCOUNT
	kn_value_free(value);
#endif /* KN_USE_REFCOUNT */

	kn_env_destroy(env);
	// kn_shutdown();

	return -1;
}
#endif /* KN_FUZZING */

kn_value kn_play(struct kn_env *env, const char *source, size_t length) {
#ifdef KN_FUZZING
	if (setjmp(kn_play_start))
		return KN_NULL;
#endif /* KN_FUZZING */

	struct kn_stream stream = {
		.source = source,
		.position = 0,
		.env = env,
		.length = length
	};

	kn_value parsed = kn_parse(stream);

	if (parsed == KN_UNDEFINED) {
#ifdef KN_FUZZING
		if (true)
			return KN_NULL;
#endif /* KN_FUZZING */
		kn_error("unable to parse stream");
	}

	kn_value ret = kn_value_run(parsed);
	kn_value_free(parsed);
	return ret;
}
