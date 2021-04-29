#pragma once
#include "bytecode.h"
#include <src/value.h>

typedef struct {
	unsigned nlocals, nglobals, nconsts, codelen;

	struct kn_variable **globals;
	kn_value *consts;
	opcode_t *code;
} frame_t;

frame_t *frame_from(kn_value);
kn_value run_frame(const frame_t *);
