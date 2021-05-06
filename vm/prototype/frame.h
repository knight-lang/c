#pragma once
#include "bytecode.h"
#include <src/value.h>

typedef struct {
	unsigned nlocals, nglobals, nconsts, codelen, refcount;

	struct kn_variable **globals;
	kn_value *consts;
	bytecode_t *code;
} frame_t;

frame_t *frame_from(kn_value);
void free_frame(frame_t *);
void dump_frame(const frame_t *);
void clone_frame(frame_t *);
kn_value run_frame(const frame_t *);
kn_value parse_and_run(const char *);
