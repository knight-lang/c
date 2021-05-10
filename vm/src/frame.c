#include "frame.h"
#include "bytecode.h"
#include <src/value.h>

struct kn_frame {
		unsigned nlocals;

	struct {
		unsigned len, cap;
		struct kn_variable **list;
	} globals;

	struct {
		unsigned len, cap;
		kn_value *list;
	} consts;

	struct {
		unsigned len, cap;
		union kn_bytecode *list;
	} code;
};

kn_value kn_frame_run(const char *stream) {
	kn_value parsed = kn_parse(stream);

	if (parsed == KN_UNDEFINED)
		die("nothing to parse.");

	struct kn_frame frame;
	parse_frame(&frame, parsed);
	kn_value_free(parsed);
	run_frame(&frame);
}


struct kn_frame *kn_frame_parse(kn_value value);
void kn_frame_dump(const struct kn_frame *frame);
void kn_frame_free(struct kn_frame *frame);
void kn_frame_clone(struct kn_frame *frame);
kn_value kn_frame_run(const struct kn_frame *);
