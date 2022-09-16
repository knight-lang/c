#include "number.h"
#include "string.h"
#include "list.h"

struct kn_string *kn_number_to_string(kn_number number) {
	static struct kn_string zero_string = KN_STRING_NEW_EMBED("0"),
		one_string = KN_STRING_NEW_EMBED("1"),
		uint64_min_string = KN_STRING_NEW_EMBED("-9223372036854775808");

	// note that `21` is the length of `INT64_MIN`, which is 20 characters
	// long + the trailing `\0`.
	static char buf[21];
	static struct kn_string number_string = { .flags = KN_STRING_FL_STATIC };

	if (number == 0)
		return &zero_string;

	if (number == 1)
		return &one_string;

	if (KN_UNLIKELY(number == INT64_MIN))
		return &uint64_min_string; // since inverting the min value doesnt work.

	// initialize ptr to the end of the buffer minus one, as the last is
	// the nul terminator.
	char *ptr = &buf[sizeof(buf) - 1];
	bool is_neg = number < 0;

	if (is_neg)
		number *= -1;

	do {
		*--ptr = '0' + (number % 10);
	} while (number /= 10);

	if (is_neg)
		*--ptr = '-';

	number_string.ptr = ptr;
	number_string.length = &buf[sizeof(buf) - 1] - ptr;

	return &number_string;
}

struct kn_list *kn_number_to_list(kn_number number) {
	static kn_value buf[20]; // note that `20` is the length of `INT64_MIN`.
	static struct kn_list digits_list = {
		.container = {
			.refcount = { 1 }
		},
		.flags = KN_LIST_FL_ALLOC | KN_LIST_FL_STATIC | KN_LIST_FL_NUMBER,
	};

	digits_list.alloc = &buf[sizeof(buf) / sizeof(kn_value)];
	digits_list.container.length = 0;

	do {
		*--digits_list.alloc = kn_value_new(number % 10);
		++digits_list.container.length;
	} while (number /= 10);

	return &digits_list;
}

