#include "memory.h"
#include "string.h"

String string_new(Arena *arena, size_t len) {
	return (String) {
		.len = len,
		.value = arena_alloc(arena, len)
	};
}
