#ifndef _MEMORYH_
#define _MEMORYH_

#include <stdio.h>
#include <stdint.h>

#include "base.h"

typedef struct Arena Arena;
struct Arena {
	u64 *buf;
	u64 len;
	u64 curr_offset;
};

uintptr_t align_forward(uintptr_t ptr, u64 align);
void *arena_alloc(Arena *a, u64 size);
void arena_init(Arena *a, void *backing_buffer, u64 backing_buffer_len);
void arena_free(Arena *a);

#endif
