#ifndef _STRINGH_
#define _STRINGH_

#include <stdio.h>

#include "memory.h"

typedef struct String String;
struct String {
	char *value;
	size_t len;
};

String string_new(Arena *arena, size_t len);

// Creates a slice from the source string. The underlying string is not copied.
// The slice returned should not be used after the source string has been freed.
// src: String to create a slice from.
// offset: Starting position from the `src` string.
// len: Length of the slice taken from the `src` string.
String string_slice(String *src, size_t offset, size_t len);

#endif
