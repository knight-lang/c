#ifndef KN_PARSE_H
#define KN_PARSE_H

#include "value.h"    /* kn_value */
#include "function.h" /* kn_function */

/*
 * The stream that all the parse functions interact with. 
 */
struct kn_stream {
	const char *source;
	struct kn_env *env;
	size_t position, length;
};

/*
 * Parses out a `kn_value` from the given stream, returning `KN_UNDEFINED` if no
 * value could be parsed.
 *
 * Note that this will overwrite the previous parsing stream. The function
 * `kn_parse_value` can be used to fetch the next value and update the parsing
 * stream.
 */
kn_value kn_parse(struct kn_stream stream);

/*
 * Attempts to parse a `kn_value` from the `kn_parse_stream`.
 *
 * Unlike the other parsing functions, this function _will_ strip leading
 * whitespace and comments.
 */
kn_value kn_parse_value(struct kn_stream *stream);

/*
 * Strips all leading whitespace and comments from the `kn_parse_stream`.
 */
void kn_parse_strip(struct kn_stream *stream);

/*
 * Parses a `kn_number` from the `kn_parse_stream`.
 *
 * The stream must start with a valid digit.
 */
kn_number kn_parse_number(struct kn_stream *stream);

/*
 * Parses a `kn_string` from the `kn_parse_stream`.
 *
 * The stream must start either a single or double quote (`'` or `"`).
 */
struct kn_string *kn_parse_string(struct kn_stream *stream);

/*
 * Parses a `kn_variable` from the `kn_parse_stream`.
 *
 * The stream must start with either a lower case letter or an underscore (`_`).
 */
struct kn_variable *kn_parse_variable(struct kn_stream *stream);

/*
 * Parses the function `function` from the `kn_parse_stream`.
 */
kn_value kn_parse_ast(struct kn_stream *stream, const struct kn_function *function);


static inline bool kn_stream_is_eof(const struct kn_stream *stream) {
	return stream->length <= stream->position;
}

/*
 * Peeks at the first character in the `kn_parse_stream`.
 */
static inline char kn_stream_peek(const struct kn_stream *stream) {
	assert(!kn_stream_is_eof(stream));
	return stream->source[stream->position];
}

/*
 * Advances the stream.
 */
static inline void kn_stream_advance(struct kn_stream *stream) {
	assert(!kn_stream_is_eof(stream));
	++stream->position;
}

#ifdef KN_CUSTOM
/*
 * This function is called whenever a keyword function starting with `X` is
 * encountered.
 *
 * The passed `stream` will have only the leading `X` removed, and the function
 * should strip the and any other relevant trailing characters before returning.
 */
extern kn_value kn_parse_extension(struct kn_stream *stream);
#endif /* KN_CUSTOM */

#endif /* !KN_PARSE_H */
