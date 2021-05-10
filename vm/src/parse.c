
#include "bytecode.h"
#include <src/value.h>


struct kn_frame {
	unsigned nlocals, nglobals, nconsts, ncode;

	struct kn_variable **globals;
	kn_value *consts;
	union kn_bytecode *code;
};


struct kn_frame *kn_frame_parse(kn_value value);
void kn_frame_dump(const struct kn_frame *frame);
void kn_frame_free(struct kn_frame *frame);
void kn_frame_clone(struct kn_frame *frame);
kn_value kn_frame_run(const struct kn_frame *);
