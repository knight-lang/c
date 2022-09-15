#include "number.h"
#include "string.h"
#include "list.h"

struct kn_string *kn_number_to_string(kn_number number) {
	static struct kn_string zero_string = KN_STRING_NEW_EMBED("0");
	static struct kn_string one_string = KN_STRING_NEW_EMBED("1");

	// note that `22` is the length of `-UINT64_MIN`, which is 21 characters
	// long + the trailing `\0`.
	static char buf[22];
	static struct kn_string number_string = { .flags = KN_STRING_FL_STATIC };

	if (number == 0)
		return &zero_string;

	if (number == 1)
		return &one_string;

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
	static struct kn_list zero_list = {
		.refcount = 1,
		.length = 1,
		.embed = { KN_ZERO },
	};

	if (number == 0) 
		return &zero_list;

	// FIXME: use base10 to find the required amount.
	struct kn_list *digits = kn_list_alloc(22);
	digits->length = 0;

	do {
		digits->alloc[digits->length++] = kn_value_new(number % 10);
	} while (number /= 10);

	// swap all the digits at the end
	for (size_t i = 0; i < digits->length / 2; ++i) {
		kn_value tmp = kn_list_get(digits, i);
		kn_list_set(digits, i, kn_list_get(digits, digits->length - i - 1));
		kn_list_set(digits, digits->length - i - 1, tmp);
	}

	return digits;
}

