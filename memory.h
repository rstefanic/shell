#include <stdio.h>
#include <stdint.h>

typedef struct Arena Arena;
struct Arena {
	unsigned char	*buf;
	size_t		len;
	size_t		curr_offset;
	size_t		prev_offset;
};

uintptr_t align_forward(uintptr_t ptr, size_t align);
void *arena_alloc(Arena *a, size_t size);
void arena_init(Arena *a, void *backing_buffer, size_t backing_buffer_len);
void arena_free(Arena *a);
