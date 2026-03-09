#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "base.h"
#include "evaluator.h"
#include "hashtable.h"
#include "lexer.h"
#include "log.h"
#include "memory.h"
#include "parser.h"
#include "string.h"

#if DEBUG
#include "debug.h"
#endif

HashTable *symtable = {0};

// Make sure the user's cursor isn't overwriting any data pinned to the top of
// the terminal. If the cursor is currently set to be within this `min_row`,
// then we'll move the cursor under it so the prompt will be fully visible.
//
// This is done by sending a Device Status Report (DSR) request to the terminal
// to get the current cursor's position. If the cursor is under the `min_row`,
// we'll adjust the cursor so the prompt is moved directly under it.
void minimum_cursor_row_start(i64 min_row) {
	// Get the termios, and turn off echo so the DSR isn't "typed" to stdout.
	struct termios curr_termios, tmp_termios;
	tcgetattr(STDIN_FILENO, &curr_termios);

	// Copy the current termios into `tmp_termios` and turn echo off.
	tmp_termios = curr_termios;
	tmp_termios.c_lflag &= (tcflag_t)~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &tmp_termios);

	// Issue DSR to ask for cursor position.
	printf("\033[6n");
	fflush(stdout);
	
	// The result comes in the format of "ESC[<row>;<col>R". We're just
	// interested in the <row> argument here and will ignore <col>.
	char dsr[32] = { 0 };	// buffer for reading the DSR
	u64 i = 0;
	char c;
	while ((c = (char)getchar()) != EOF && i < 31) {
		dsr[i++] = c;
		if (c == 'R')
			break;
	}

	char *end; // end pointer for `strtol` call
	i64 row = strtol(dsr+3, &end, 10);
	assert(end != dsr);

	// If the cursor is in the min row, then move it under the min_row.
	if (row < min_row) {
		printf("\033[%llu;1H\033[K", min_row+1);
	}

	// Restore the old termios.
	tcsetattr(STDIN_FILENO, TCSANOW, &curr_termios);
}

int main(void) {
	unsigned char perm_arena_backing_buffer[MB(2)];
	Arena perm_arena = {0};
	arena_init(&perm_arena, perm_arena_backing_buffer, MB(2));
	assert(perm_arena.buf != NULL);

	// Configure runtime.
	Arena runtime_arena = {0};
	arena_init(&runtime_arena, arena_alloc(&perm_arena, KB(32)), KB(32));
	assert(runtime_arena.buf != NULL);
	Runtime runtime = create_runtime(&runtime_arena);

	// Setup arena for main REPL frame.
	Arena frame_arena = {0};
	arena_init(&frame_arena, arena_alloc(&perm_arena, KB(128)), KB(128));
	assert(frame_arena.buf != NULL);

	// Main repl. Each run of this is considered a "frame".
	for(;;) {
		log_context_start();

		arena_free(&frame_arena);
		String input = string_new(&frame_arena, 256);

		printf("> ");
		fgets(input.value, (i32)input.len, stdin);

		u64 token_len = 256;
		Token *tokens = arena_alloc(&frame_arena, token_len * sizeof(Token));
		assert(tokens != NULL);

		bool ok = lex(tokens, token_len, &input);
		if (!ok) {
			goto cleanup;
		}

		Expression *expressions = parse(&frame_arena, tokens, token_len);

	#if DEBUG
		print_tokens(tokens, token_len);
		print_expressions(expressions, 0);
	#endif

		assert(expressions->type == EXPR_LIST);
		evaluator_eval_expressions(&runtime, expressions);

		cleanup:
		if (log_context_count_by(LOG_LEVEL_INFO) > 0) {
			// Print the Info Report section at the top of the
			// terminal. Save the cursor position, jump to the top,
			// print the section, and restore the current cursor.
			String messages = log_context_get_messages(LOG_LEVEL_INFO);
			printf("\33[s");	// save the cursor position
			printf("\033[H");	// put the cursor top left
			printf("\033[K");	// clear to end of line

			char *green_foreground = "\033[38;5;46m";
			printf("%s=== INFO REPORT ===\033[K\n", green_foreground);

			i64 info_line_count = 1;
			for (u64 i = 0; i < messages.len; i++) {
				char c = messages.value[i];
				if (c == '\n') {
					printf("\033[K"); // clear to end of line
					info_line_count++;
				}
				putchar(c);
			}

			char *reset_colors = "\033[0m";
			printf("=== END INFO REPORT ===\033[K%s\n", reset_colors);
			info_line_count++;

			printf("\033[u");	// restore cursor position
			minimum_cursor_row_start(info_line_count);
		}

		log_context_end();
	}

	return 0;
}
