#include <assert.h>
#include <stdbool.h>

#include "string.h"

String string_new(Arena *arena, size_t len) {
	char *value = arena_alloc(arena, len);
	assert(value != NULL);

	return (String) {
		.len = len,
		.value = value
	};
}

String string_slice(String *src, size_t offset, size_t len) {
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

	for (size_t i = 0; i < a->len; i++) {
		if (a->value[i] != b->value[i]) {
			return false;
		}
	}

	return true;
}
