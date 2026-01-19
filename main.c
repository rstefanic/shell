#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "memory.h"

typedef enum BuiltinCommand BuiltinCommand;
enum BuiltinCommand {
	NONE,
	CD,
	EXIT
};

BuiltinCommand parse_builtin(char *input) {
	if (memcmp(input, "cd", 2) == 0) {
		return CD;
	} else if (memcmp(input, "exit", 4) == 0) {
		return EXIT;
	}

	return NONE;
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

		// DEBUG: Print tokens
		for (size_t i = 0; i < token_len; i++) {
			Token tok = tokens[i];
			if (tok.type == EMPTY)
				break;

			printf("type: %d, raw: [%.*s]\n",
				tok.type,
				(int)tok.len,
				tok.raw);
		}

		switch(parse_builtin(input)) {
		case CD:
			printf("BUILTIN cd: %s\n", &input[2]);
			break;
		case EXIT:
			printf("BUILTIN: exit\n");
			return 0;
		case NONE:
			printf("%s\n", input);
			break;
		}
	}

	return 0;
}
