#include <stdio.h>
#include <string.h>

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
	size_t backing_buffer_len = 1024;
	unsigned char backing_buffer[backing_buffer_len];

	Arena a = {0};
	arena_init(&a, backing_buffer, backing_buffer_len);

	for(;;) {
		arena_free(&a);

		size_t input_len = 256;
		char* input = arena_alloc(&a, input_len);

		printf("> ");
		fgets(input, input_len, stdin);

		switch(parse_builtin(input)) {
		case CD:
			printf("\nChange directory: %s\n", &input[2]);
			break;
		case EXIT:
			return 0;
		case NONE:
			printf("\n%s\n", input);
			break;
		}
	}

	return 0;
}
