#ifndef _LOGH_
#define _LOGH_

#include <assert.h>

#include "base.h"
#include "string.h"

#define LOG_ARENA_BYTES_LEN 1024 * 32 // 32kb

typedef enum {
	LOG_LEVEL_NONE = 0,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR
} LogLevel;

typedef struct {
	String message;
	LogLevel level;
} LogMessage;

typedef struct {
	Arena *arena;
	u64 size;
	u64 capacity;
	LogMessage *messages;
} LogContext;

void log_context_start();
void log_context_end();
void log_emit_message(LogLevel level, String message);
u64 log_context_count_by(LogLevel level);
String log_context_get_messages(LogLevel level);

#endif
