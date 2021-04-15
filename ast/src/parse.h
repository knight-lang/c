#ifndef KN_PARSE_H
#define KN_PARSE_H

#include "value.h"    /* kn_value */
#include "function.h" /* kn_function */

/*
 * Parses out a `kn_value` from the given stream, returning `KN_UNDEFINED` if no
 * value could be parsed.
 *
 * Note that this will overwrite the previous parsing stream. The function 
 * `kn_parse_value` can be used to fetch the next value and update the parsing
 * stream.
 */
kn_value kn_parse(const char *stream);

/*
 * The stream that all the parse functions interact with. This is set by the
 * `kn_parse` function.
 */
extern const char *kn_parse_stream;

/*
 * Attempts to parse a `kn_value` from the `kn_parse_stream`.
 *
 * Unlike the other parsing functions, this function _will_ strip leading
 * whitespace and comments.
 */
kn_value kn_parse_value(void);

/*
 * Strips all leading whitespace and comments from the `kn_parse_stream`.
 */
void kn_parse_strip(void);

/*
 * Parses a `kn_number` from the `kn_parse_stream`.
 *
 * The stream must start with a valid digit.
 */
kn_number kn_parse_number(void);

/*
 * Parses a `kn_string` from the `kn_parse_stream`.
 *
 * The stream must start either a single or double quote (`'` or `"`).
 */
struct kn_string *kn_parse_string(void);

/*
 * Parses a `kn_variable` from the `kn_parse_stream`.
 *
 * The stream must start with either a lower case letter or an underscore (`_`).
 */
struct kn_variable *kn_parse_variable(void);

/*
 * Parses the function `function` from the `kn_parse_stream`.
 */
struct kn_ast *kn_parse_ast(const struct kn_function *function);

/*
 * Peeks at the first character in the `kn_parse_stream`.
 */
static inline char kn_parse_peek() {
	return *kn_parse_stream;
}

/*
 * Advances the stream.
 */
static inline void kn_parse_advance() {
	++kn_parse_stream;
}

/*
 * Advances the stream, then fetches the first character of the stream.
 */
static inline char kn_parse_advance_peek() {
	return *++kn_parse_stream;
}

/*
 * Fetches the first character of the stream, then advances it.
 */
static inline char kn_parse_peek_advance() {
	return *kn_parse_stream++;
}


#ifdef KN_CUSTOM
/*
 * This function is called whenever a keyword function starting with `X` is
 * encountered.
 *
 * The passed `stream` will have only the leading `X` removed, and the function
 * should strip the and any other relevant trailing characters before returning.
 */
extern kn_value kn_parse_extension(void);
#endif /* KN_CUSTOM */

#endif /* !KN_PARSE_H */
