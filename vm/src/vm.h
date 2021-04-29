#pragma once
#include "frame.h"
#include <src/value.h>

typedef struct {
	unsigned ip;
} vm_t;

void vm_init(vm_t *);
kn_value vm_run(vm_t *, frame_t *);
