#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "string.h"

String string_new(Arena *arena, u64 len) {
	char *value = arena_alloc(arena, len);
	assert(value != NULL);

	return (String) {
		.len = len,
		.value = value
	};
}

String string_copy(Arena *arena, String existing) {
	String new = string_new(arena, existing.len);
	memcpy(new.value, existing.value, new.len);
	return new;
}

String string_slice(String *src, u64 offset, u64 len) {
	assert(len <= src->len);
	assert(offset <= src->len);

	return (String) {
		.len = len,
		.value = src->value+offset
	};
}

bool string_compare(String *a, String *b) {
	if (a->len != b->len) {
		return false;
	}

	for (u64 i = 0; i < a->len; i++) {
		if (a->value[i] != b->value[i]) {
			return false;
		}
	}

	return true;
}
