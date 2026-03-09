#include "log.h"
#include "memory.h"
#include "string.h"
#include <string.h>

static __thread u8 _global_log_arena_backing_buffer[LOG_ARENA_BYTES_LEN];
static __thread Arena _global_log_arena = { 0 };
static __thread LogContext _global_log_context = { 0 };

// First time setup for the _global_log_context. Adds the reference to the
// arena so that we can allocate and easily clean up the global log context.
void init_global_log_context(void) {
	if (_global_log_arena.buf == NULL) {
		arena_init(
			&_global_log_arena,
			_global_log_arena_backing_buffer,
			LOG_ARENA_BYTES_LEN
		);

		_global_log_context.arena = &_global_log_arena;
		_global_log_context.messages = NULL;
	}
}

void log_context_start(void) {
	// Ensure that we're not re-starting a context that's already open.
	assert(_global_log_context.messages == NULL);
	init_global_log_context();

	_global_log_context.size = 0;
	_global_log_context.capacity = 10;
	_global_log_context.arena = &_global_log_arena;
	_global_log_context.messages = arena_alloc(
		_global_log_context.arena,
		sizeof(LogMessage) * _global_log_context.capacity
	);
}

void log_context_end(void) {
	assert(_global_log_context.messages != NULL);
	arena_free(_global_log_context.arena);
	_global_log_context.messages = NULL;
}

void log_emit_message(LogLevel level, String message) {
	assert(_global_log_context.messages != NULL);
	assert(_global_log_context.size < _global_log_context.capacity);

	// Grab the current context size and increment it.
	u64 idx = _global_log_context.size;
	_global_log_context.size += 1;

	// Add this log message copying the existing message. Since the scope
	// of the String passed in is not guaranteed to have the same lifetime
	// as this context, we'll copy the String to avoid invalid memory reads.
	LogMessage *log_message = &_global_log_context.messages[idx];
	log_message->level = level;
	log_message->message = string_copy(&_global_log_arena, message);
}

u64 log_context_count_by(LogLevel level) {
	// Create an array for the 4 LogLevels and count them.
	u64 context_counts[4] = { 0 };
	for (u64 i = 0; i < _global_log_context.size; i++) {
		LogMessage message = _global_log_context.messages[i];
		context_counts[message.level] += 1;
	}

	return context_counts[level];
}

String log_context_get_messages(LogLevel level) {
	if (_global_log_context.size == 0) {
		return STR_LIT("");
	}
	
	u64 level_count = log_context_count_by(level);
	if (level_count == 0) {
		return STR_LIT("");
	}

	// We're assuming here that the log messages are under 256 bytes.
	u64 result_pos = 0;
	String result = string_new(_global_log_context.arena, level_count * 256);
	for (u64 i = 0; i < _global_log_context.size; i++) {
		LogMessage message = _global_log_context.messages[i];
		if (message.level == level) {
			assert((result_pos + message.message.len) < result.len);
			memcpy(result.value + result_pos, message.message.value, message.message.len);
			result_pos += message.message.len;
		}
	}

	result.len = result_pos;
	return result;
}
