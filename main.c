#include <assert.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lexer.h"
#include "memory.h"

typedef enum BuiltinCommand BuiltinCommand;
enum BuiltinCommand {
	NONE,
	CD,
	PWD,
	EXIT
};

BuiltinCommand try_parse_builtin(Token *tok) {
	char buf[256];
	assert(tok->len < 256);
	memcpy(buf, tok->raw, tok->len);

	if (memcmp(buf, "cd", 2) == 0) {
		return CD;
	} else if (memcmp(buf, "pwd", 3) == 0) {
		return PWD;
	} else if (memcmp(buf, "exit", 4) == 0) {
		return EXIT;
	}

	return NONE;
}

void handle_builtin(Token *tokens, BuiltinCommand type) {
	Token tok = tokens[0];

	// Pull out the current working directory and store it in path. Some
	// builtin commands need to manipulate the path so setting this to the
	// current working directory is a sensible default.
	char path[PATH_MAX] = {0};
	char *res = getcwd(path, PATH_MAX);
	assert(res != NULL);

	switch(type) {
	case CD: {
		// Set the path to the directory specified by the user if it's
		// an absolute path. Zero out the remaining contents of the path
		// buffer so that we avoid conflicts with the user's directory.
		if (tok.raw[0] == '/') {
			memcpy(path, tok.raw, tok.len);
			memset(path+tok.len, 0, PATH_MAX-tok.len);
		} else {
			// Handle relative path navigation.
			unsigned long pathlen = strlen(path);

			// If the current path doesn't have a trailing '/',
			// add it before appending the relative path to path.
			if (path[pathlen] != '/') {
				path[pathlen] = '/';
				pathlen += 1;
			}

			assert((pathlen+tok.len) < PATH_MAX);
			memcpy(&path[pathlen], tok.raw, tok.len);
		}
		int ok = chdir(path);
		if (ok != 0) {
			printf("cd: could not find directory \"%s\"\n", path);
		}
		break;
	}
	case PWD: {
		printf("%s\n", path);
		break;
	}
	default:
		assert(false); // unreachable
	}
}

void print_tokens(Token *tokens, size_t token_len) {
	size_t i = 0;
	for (i = 0; i < token_len; i++) {
		Token tok = tokens[i];

		// Stop as soon as we hit our first empty token.
		if (tok.type == EMPTY)
			break;

		printf("type: %d, raw: (%.*s)\n",
			tok.type,
			(int)tok.len,
			tok.raw);
	}
}

int main() {
	size_t backing_buffer_len = 1024 * 10; // 10kb
	unsigned char backing_buffer[backing_buffer_len];

	Arena a = {0};
	arena_init(&a, backing_buffer, backing_buffer_len);

	for(;;) {
		arena_free(&a);

		size_t input_len = 256;
		char* input = arena_alloc(&a, input_len);

		printf("> ");
		fgets(input, input_len, stdin);

		size_t token_len = 256;
		Token *tokens = arena_alloc(&a, token_len * sizeof(Token));
		lex(tokens, token_len, input, strlen(input));

		Token *tok = &tokens[0];
		assert(tok->type != EMPTY);

		if (tok->type == LITERAL) {
			BuiltinCommand cmd = try_parse_builtin(tok);
			if (cmd == NONE) {
				print_tokens(tokens, token_len);
			} else if (cmd == EXIT) {
				break;
			} else {
				// We already know the first token was this
				// builtin command, so we'll pass in the
				// remaining tokens to be handled.
				handle_builtin(&tokens[1], cmd);
			}
		}
	}

	return 0;
}
