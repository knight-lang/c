char *strerror(int);
extern int errno;
unsigned long strlen(const char *);
#include "vm.h"
#include "alloc.h"
#include "value.h"
#include "list.h"

#define kn_error(...) kn_die(__VA_ARGS__)

struct kn_stackframe {
	const struct kn_vm *vm;
	size_t ip;
};

static void push(struct kn_vm *vm, kn_value value) {
	if (KN_UNLIKELY(vm->stack_length == vm->stack_cap))
		vm->stack = kn_heap_realloc(vm->stack, vm->stack_cap *= 2);
	vm->stack[vm->stack_length++] = value;
}

static kn_value pop(struct kn_vm *vm) {
	kn_assertm(vm->stack_length != 0, "bug, vm stack popped when length is 0%s", "");
	return vm->stack[--vm->stack_length];
}

static union kn_bytecode next(struct kn_vm *vm, size_t *ip) {
	kn_assert(vm->bytecode_length < *ip);
	return vm->bytecode[*ip++];
}

static kn_value prompt(void);

kn_value kn_vm_run(struct kn_vm *vm, size_t ip) {
	kn_value args[KN_OPCODE_MAX_ARITY];
	unsigned int offset;

	while (1) {
		kn_assert(ip <= vm->bytecode_length);
		enum kn_opcode oc = next(vm, &ip).opcode;

		for (unsigned i = 0; i < kn_opcode_arity(oc); ++i)
			args[i] = pop(vm);

		if (kn_opcode_takes_offset(oc))
			offset = next(vm, &ip).offset;

		switch(oc) {
		case KN_OPCODE_PUSH_CONSTANT:
			kn_assert(offset <= vm->constants_length);
			push(vm, vm->constants[offset]);
			break;

		case KN_OPCODE_POP_AND_RET:
			return args[0];

		case KN_OPCODE_JEQ:
		case KN_OPCODE_JNE:
			if (kn_value_to_boolean(args[0]) == (oc == KN_OPCODE_JEQ))
			case KN_OPCODE_JMP:
				ip = offset;
			break;

		case KN_OPCODE_PROMPT:
			push(vm, prompt());
			break;

		case KN_OPCODE_RANDOM:
			push(vm, kn_value_new_integer((kn_integer) rand()));
			break;

		case KN_OPCODE_CALL:
			kn_die("todo KN_OPCODE_CALL");//   = KN_NEW_OPCODE(1, 0,  0),

		case KN_OPCODE_QUIT: {
			int status_code = (int) kn_value_to_integer(args[0]);

#ifdef KN_FUZZING
			longjmp(kn_play_start, 1);
#else
			exit(status_code);
# endif /* KN_FUZZING */
		}


		case KN_OPCODE_OUTPUT: {
			struct kn_string *string = kn_value_to_string(args[0]);
			char *str = kn_string_deref(string);

		#ifndef KN_RECKLESS
			clearerr(stdout);
		#endif /* !KN_RECKLESS */

			size_t length = string->length;
			if (length != 0 && str[length - 1] == '\\') {
				fwrite(str, sizeof(char), length - 1, stdout);
			} else {
				fwrite(str, sizeof(char), length, stdout);
				putchar('\n');
			}

			fflush(stdout);
			if (ferror(stdout))
				kn_error("unable to write to stdout");

			push(vm, KN_NULL);
			continue;
		}

		case KN_OPCODE_DUMP:
			kn_value_dump(args[0], stdout);
			push(vm, args[0]);
			break;

		case KN_OPCODE_LENGTH:
			if (KN_LIKELY(kn_value_is_list(args[0]) || kn_value_is_string(args[0]))) {
				if (kn_value_is_list(args[0]))
					push(vm, kn_value_new_integer((kn_integer) kn_value_as_list(args[0])->length));
				else
					push(vm, kn_value_new_integer((kn_integer) kn_value_as_string(args[0])->length));
				continue;
			}

			if (kn_value_is_integer(args[0])) {
				kn_integer integer = kn_value_as_integer(args[0]);
				kn_integer length = 0;

				do {
					length++;
					integer /= 10;
				} while (integer);

				push(vm, kn_value_new_integer(length));
				continue;
			}

			push(vm, kn_value_new_integer(kn_value_to_list(args[0])->length));
			continue;

		case KN_OPCODE_NOT:
			push(vm, kn_value_new_boolean(!kn_value_to_boolean(args[0])));
			break;

		case KN_OPCODE_NEGATE:
			push(vm, kn_value_new_integer(-kn_value_to_integer(args[0])));
			break;

		case KN_OPCODE_ASCII: {
			if (!kn_value_is_integer(args[0]) && !kn_value_is_string(args[0]))
				kn_error("can only call ASCII on integer and strings.");

			// If lhs is a string, convert both to a string and concatenate.
			if (kn_value_is_string(args[0])) {
				struct kn_string *string = kn_value_as_string(args[0]);

				if (string->length == 0)
					kn_error("ASCII called with empty string.");

				char head = kn_string_deref(string)[0];

				push(vm, kn_value_new_integer(head));
				continue;
			}

			kn_integer integer = kn_value_as_integer(args[0]);

			// just check for ASCIIness, not actually full-blown knight correctness. 
			if (integer <= 0 || 127 < integer)
				kn_error("Integer %" PRIdkn " is out of range for ascii char.", integer);

			char buf[2] = { (char) integer, 0 };
			push(vm, kn_value_new_string(kn_string_new_borrowed(buf, 1)));
			break;
		}

		case KN_OPCODE_BOX:;
			struct kn_list *list = kn_list_alloc(1);

			kn_assert(kn_flags(list) & KN_LIST_FL_EMBED);
			list->embed[0] = args[0];

			push(vm, kn_value_new_list(list));
			break;

		case KN_OPCODE_HEAD:
			if (!kn_value_is_string(args[0]) && !kn_value_is_list(args[0]))
				kn_error("can only call `[` on strings and lists");

			if (kn_value_is_list(args[0])) {
				struct kn_list *list = kn_value_as_list(args[0]);
				if (list->length == 0) kn_error("called `[` on empty list/string");
				kn_value head = kn_list_get(list, 0);

				push(vm, head);
			} else {
				struct kn_string *string = kn_value_as_string(args[0]);
				if (string->length == 0) kn_error("called `[` on empty list/string");
				struct kn_string *head = kn_string_new_borrowed(kn_string_deref(string), 1);

				push(vm, kn_value_new_string(head));
			}
			break;

		case KN_OPCODE_TAIL:
			if (!kn_value_is_string(args[0]) && !kn_value_is_list(args[0]))
				kn_error("can only call `]` on strings and lists");

			if (kn_value_is_list(args[0])) {
				struct kn_list *list = kn_value_as_list(args[0]);
				if (list->length == 0) kn_error("called `]` on an empty list/string");
				push(vm, kn_value_new_list(kn_list_get_sublist(list, 1, list->length - 1)));
			} else {
				struct kn_string *string = kn_value_as_string(args[0]);
				if (string->length == 0) kn_error("called `]` on an empty list/string");
				push(vm, kn_value_new_string(kn_string_get_substring(string, 1, string->length - 1)));
			}
			break;

		case KN_OPCODE_ADD: kn_die("todo KN_OPCODE_ADD");// = KN_NEW_OPCODE(2, 0, 0),
		case KN_OPCODE_SUB: kn_die("todo KN_OPCODE_SUB");// = KN_NEW_OPCODE(2, 0, 1),
		case KN_OPCODE_MUL: kn_die("todo KN_OPCODE_MUL");// = KN_NEW_OPCODE(2, 0, 2),
		case KN_OPCODE_DIV: kn_die("todo KN_OPCODE_DIV");// = KN_NEW_OPCODE(2, 0, 3),
		case KN_OPCODE_MOD: kn_die("todo KN_OPCODE_MOD");// = KN_NEW_OPCODE(2, 0, 4),
		case KN_OPCODE_POW: kn_die("todo KN_OPCODE_POW");// = KN_NEW_OPCODE(2, 0, 5),
		case KN_OPCODE_LTH: kn_die("todo KN_OPCODE_LTH");// = KN_NEW_OPCODE(2, 0, 6),
		case KN_OPCODE_GTH: kn_die("todo KN_OPCODE_GTH");// = KN_NEW_OPCODE(2, 0, 7),
		case KN_OPCODE_EQL: kn_die("todo KN_OPCODE_EQL");// = KN_NEW_OPCODE(2, 0, 8),

		case KN_OPCODE_GET: kn_die("todo KN_OPCODE_GET");// = KN_NEW_OPCODE(3, 0, 0),
		case KN_OPCODE_SET: kn_die("todo KN_OPCODE_SET");// = KN_NEW_OPCODE(4, 0, 0),
		default:
			kn_bug("unknown opcode");
		}
	}
}

kn_value prompt(void) {
#ifdef KN_FUZZING
	// Don't read from stdin during fuzzing.
	if (true)
		return kn_value_new(&kn_string_empty);
#endif /* KN_FUZZING */

	if (KN_UNLIKELY(feof(stdin)))
		return KN_NULL; // avoid the allocation.

	int length = 0;
	int capacity = 1024;
	char *line = kn_heap_malloc(capacity);

	while (1) {
		if (!fgets(line + length, capacity - length, stdin)) {
			if (ferror(stdin)) {
				KN_MSVC_SUPPRESS(4996)
				kn_error("unable to read line from stdin: %s", strerror(errno));
			}

			kn_assert(feof(stdin));
			kn_heap_free(line);
			return KN_NULL;
		}

		length += strlen(line + length);
		if (length != capacity - 1)
			break;
		line = kn_heap_realloc(line, capacity *= 2);
	}

	kn_assert(length != 0); // shoudla been checked by fgets
	if (line[length - 1] == '\n')
		length--;

	while (length && line[length - 1] == '\r')
		--length;

	line[length] = '\0';

	if (KN_UNLIKELY(length == 0)) {
		kn_heap_free(line);
		return kn_value_new_string(&kn_string_empty);
	}

	return kn_value_new_string(kn_string_new_owned(line, length));
}
