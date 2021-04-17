#ifndef EXT_H
#define EXT_H

#include "../src/value.h"
#include "../src/parse.h"
#include "../src/shared.h"
#include <stddef.h>
#include <string.h>

kn_value kn_parse_extension(void);

// kn_value parse_value(char skip);
void strip_keyword(void);
bool stream_starts_with(const char *str);
bool stream_starts_with_strip(const char *str);

void unsupported_function(void *);

static inline void *memdup(void *ptr, size_t length) {
	return memcpy(xmalloc(length), ptr, length);
}

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type,member)))
#define DATA2VALUE(data_) kn_value_new_custom(container_of(data_, struct kn_custom, data))
#define VALUE2DATA(value) ((void *) kn_value_as_custom(value)->data)
#define ALLOC_DATA(size, vtable) ((void *) kn_custom_alloc(size, vtable)->data)

#define TRY_PARSE_FUNCTION(string, function) \
	if (stream_starts_with_strip(string)) \
		return kn_value_new_ast(kn_parse_ast(&function));

#define KN_CUSTOM_UNDEFINED(val) (((kn_value) (val)) << 5)

#endif /* EXT_H */
