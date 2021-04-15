#ifndef KN_EXT_H
#define KN_EXT_H

#include "../src/value.h"

kn_value kn_parse_extension(void);

// kn_value parse_value(char skip);
void strip_keyword(void);
bool stream_starts_with(const char *str);
bool stream_starts_with_strip(const char *str);

#endif /* KN_EXT_H */