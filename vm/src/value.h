#ifndef VALUE_H
#define VALUE_H

typedef long long number;
typedef unsigned long long value;

#define V_NULL 2
#define V_TRUE 4
#define V_FALSE 0

#define TAG_NUMBER 1
#define TAG_STRING 2

#define NUMBER_TO_VALUE(num) ((((value) (num)) << 8) | TAG_NUMBER)
#define STRING_TO_VALUE(str) (((value) (str)) | TAG_STRING)
#define VALUE_TO_NUMBER(val) (((number) (val)) >> 8)
#define VALUE_TO_STRING(str) ((char *) ((str) & ~TAG_STRING))

void value_dump(value);

#endif
