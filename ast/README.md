# Knight in C
This is the AST-walker implementation of Knight in C. It's (as of April 2021), the most performant version of Knight I've developed.

todo: make the old `assert_reckless`s into actual errors

# Macros
## Micro-optimizations
- `KN_ENV_NBUCKETS`: Used to adjust how many buckets the environment uses when hashing.
- `KN_ENV_CAPACITY`: Used to adjust the amount of variables that can exist in each bucket.
- `KN_STRING_PADDING_LENGTH`: Used to adjust the amount of extra padding given to embedded strings. This is generally chosen to be a number that rounds off the string's length to a multiple of two.
- `KN_USE_EXTENSIONS`: Enables the use of compiler extensions, such as `__attribute__` and `__builtin_expect`. This does not imply `KN_COMPUTED_GOTOS`, and both need to be defined separately.
- `KN_COMPUTED_GOTOS`: Enables the use of computed gotos, which can significantly increase the speed of the parsing functions. However, since this uses nonstandard features, it's not enabled by default.
- `KN_STRING_CACHE_MAXLEN`: Can control the maximum length string that will be cached.
- `KN_STRING_CACHE_LINELEN`: The power of strings per length that can be cached. Should be an even multiple of two

## Macro-optimizations
- `NDEBUG`: Disables all _internal_ debugging code. This should only be undefined when debugging.
- `KN_RECKLESS`: Assumes that absolutely no problems will occur during the execution of the program. _All_ checks for undefined behaviour are completely removed (including things like "did the `` ` `` function open properly?").

## Extensions
- `KN_EXT_VALUE`: Enables the use of the `VALUE` function, which looks up a variable indirectly based on its argument.
- `KN_EXT_NEGATE`: Enables the use of the `~` function, which simply converts its argument to a number and negates it.
- `KN_EXT_EQL_INTERPOLATE`: Allows the use of non-identifiers on the LHS of `=`, which will be coerced to an identifier.
- `KN_EXT_CUSTOM_TYPES`
- `KN_EXT_FUNCTION`