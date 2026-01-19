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

	switch(type) {
	case CD:
		printf("BUILTIN cd: %.*s\n", (int)tok.len, tok.raw);
		break;
	case PWD: {
		char cwd[PATH_MAX];
		char *res = getcwd(cwd, PATH_MAX);
		assert(res != NULL);
		printf("%s\n", cwd);
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
