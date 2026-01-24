#ifndef _STRINGH_
#define _STRINGH_

#include <stdio.h>

#include "memory.h"

typedef struct String String;
struct String {
	char *value;
	size_t len;
};

struct String string_new(Arena *arena, size_t len);

#endif
