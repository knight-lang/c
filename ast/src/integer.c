#include "integer.h"
#include "string.h"
#include "list.h"

struct kn_string *kn_integer_to_string(kn_integer integer) {
	static struct kn_string zero_string = KN_STRING_NEW_EMBED("0"),
		one_string = KN_STRING_NEW_EMBED("1"),
		uint64_min_string = KN_STRING_NEW_EMBED("-9223372036854775808");

	// note that `21` is the length of `INT64_MIN`, which is 20 characters
	// long + the trailing `\0`.
	static char buf[21];
	static struct kn_string integer_string = { .flags = KN_STRING_FL_STATIC };

	if (integer == 0)
		return &zero_string;

	if (integer == 1)
		return &one_string;

	if (KN_UNLIKELY(integer == INT64_MIN))
		return &uint64_min_string; // since inverting the min value doesnt work.

	// initialize ptr to the end of the buffer minus one, as the last is
	// the nul terminator.
	char *ptr = &buf[sizeof(buf) - 1];
	bool is_neg = integer < 0;

	if (is_neg)
		integer *= -1;

	do {
		*--ptr = '0' + (integer % 10);
	} while (integer /= 10);

	if (is_neg)
		*--ptr = '-';

	integer_string.ptr = ptr;
	kn_length_set(&integer_string, &buf[sizeof(buf) - 1] - ptr);

	return &integer_string;
}

struct kn_list *kn_integer_to_list(kn_integer integer) {
	static kn_value buf[20]; // note that `20` is the length of `INT64_MIN`.
	static struct kn_list digits_list = {
		.container = {
			.refcount = { 1 }
		},
		.flags = KN_LIST_FL_ALLOC | KN_LIST_FL_STATIC | KN_LIST_FL_INTEGER,
	};

	digits_list.alloc = &buf[sizeof(buf) / sizeof(kn_value)];
	kn_length_set(&digits_list, 0);

	do {
		*--digits_list.alloc = kn_value_new(integer % 10);
		++digits_list.container.length;
	} while (integer /= 10);

	return &digits_list;
}
