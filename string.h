#ifndef _STRINGH_
#define _STRINGH_

#include <stdio.h>
#include <stdbool.h>

#include "base.h"
#include "memory.h"

#define STR_LIT(s) (String){ s, sizeof(s) - 1 }

typedef struct String String;
struct String {
	char *value;
	u32 len;
};

String string_new(Arena *arena, u32 len);

// Copies the string into a new arena.
String string_copy(Arena *arena, String existing);

// Creates a slice from the source string. The underlying string is not copied.
// The slice returned should not be used after the source string has been freed.
// src: String to create a slice from.
// offset: Starting position from the `src` string.
// len: Length of the slice taken from the `src` string.
String string_slice(String *src, u32 offset, u32 len);

bool string_compare(String *a, String *b);

#endif
