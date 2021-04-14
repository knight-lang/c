#ifndef KN_PARSE_H
#define KN_PARSE_H

#include "value.h"    /* kn_value */
#include "function.h" /* kn_function */

/*
 * Parses out a value from the given stream, updating the stream to reflect the
 * new value.
 *
 * If no value can be parsed, `KN_UNDEFINED` will be returned.
 */
kn_value kn_parse(const char **stream);

void kn_parse_strip(const char **stream);
kn_number kn_parse_number(const char **stream);
struct kn_string *kn_parse_string(const char **stream);
struct kn_variable *kn_parse_variable(const char **stream);
struct kn_ast *kn_parse_ast(const struct kn_function *fn, const char **stream);

#ifdef KN_CUSTOM
/*
 * This function is called whenever a keyword function starting with `X` is
 * encountered.
 *
 * The passed `stream` will have only the leading `X` removed, and the function
 * should strip the and any other relevant trailing characters before returning.
 */
kn_value kn_parse_extension(const char **stream);
#endif /* KN_CUSTOM */

#endif /* !KN_PARSE_H */
