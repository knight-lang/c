#ifndef KN_ISEQ
#define KN_ISEQ

#include "types.h"
#include "opcode.h"

// `iseq` is an instruction sequence`

struct kn_iseq {
	size_t variables_length;

#ifndef KN_NDEBUG
	size_t bytecode_length, constants_length;
#endif

	const kn_value *constants;
	union kn_bytecode *bytecode;
};

// defined in `parse.c`
struct kn_iseq kn_iseq_parse(const char *source, size_t length);

#endif /* KN_ISEQ */
