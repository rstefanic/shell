#ifndef _MEMORYH_
#define _MEMORYH_

#include <stdio.h>
#include <stdint.h>

#include "base.h"

typedef struct Arena Arena;
struct Arena {
	unsigned char	*buf;
	u32		len;
	u32		curr_offset;
};

uintptr_t align_forward(uintptr_t ptr, u32 align);
void *arena_alloc(Arena *a, u32 size);
void arena_init(Arena *a, void *backing_buffer, u32 backing_buffer_len);
void arena_free(Arena *a);

#endif
