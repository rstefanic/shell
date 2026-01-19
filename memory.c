#include <stdio.h>
#include <stdint.h>

#include "memory.h"

uintptr_t align_forward(uintptr_t ptr, size_t align) {
	uintptr_t p, a, diff;

	// Calculate the number of bytes we currently are from our alignment.
	p = ptr;
	a = (uintptr_t)align;
	diff = p % a;

	// If there is a difference, then we are not aligned and we'll move ptr
	// forward so that it's aligned in memory.
	if (diff != 0) {
		p += a - diff;
	}

	return p;
}

void *arena_alloc(Arena *a, size_t size) {
	uintptr_t alignment = 2 * sizeof(void *); // machine's word size

	// Get the address of the current offset from the buffer, align it
	// forward to line up with a WORD, and convert it back to a relative
	// pointer for the arena's buffer.
	uintptr_t curr_ptr = (uintptr_t)a->buf + (uintptr_t)a->curr_offset;
	uintptr_t offset = align_forward(curr_ptr, alignment);
	offset -= (uintptr_t)a->buf;

	// Check if there's enough space and return NULL if we're out of space.
	if (offset+size > a->len) {
		return NULL;
	}

	// Allocate a new chunk and return its position.
	void *ptr = &a->buf[offset];
	a->curr_offset = offset + size;
	return ptr;
}

void arena_init(Arena *a, void *backing_buffer, size_t backing_buffer_len) {
	a->buf = (unsigned char *)backing_buffer;
	a->len = backing_buffer_len;
	a->curr_offset = 0;
}

void arena_free(Arena *a) {
	a->curr_offset = 0;
}
