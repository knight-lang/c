#include "hash.h"
#include "integer.h"
#include "alloc.h"
#include "types.h"
#include "value.h"
#include "opcode.h"
#include "parse.h"
#include "vm.h"

int main(int argc, char**argv) {
	(void) argc;

	unsigned long strlen(const char *);
	char *c = "; = x 3 D x";
	c = argv[2];
	struct kn_vm vm = kn_parse(c, strlen(c));
	vm.stack_length = 0;
	vm.stack = kn_heap_alloc_ary(vm.stack_cap = 8, kn_value);
	kn_vm_run(&vm, 0);
}
