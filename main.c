#include <assert.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
		} else if (tok.raw[0] == '~') {
			const char* home = getenv("HOME");
			size_t homelen = strlen(home);
			size_t totallen = homelen;
			memcpy(path, home, homelen);

			if (tok.len > 1) {
				memcpy(&path[homelen], &tok.raw[1], tok.len - 1);
				totallen += tok.len - 1;
			}
			memset(path+totallen, 0, PATH_MAX-totallen);
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

void execute_program(Token *tokens, size_t token_len) {
	Token tok = tokens[0];
	char pathbuf[1024];	// copy of the PATH environment variable
	char *path;		// for the PATH environment variable
	char *saveptr;		// to maintain context between strtok_r calls
	char *curr;		// current path that's being checked

	// Make sure this token isn't empty.
	assert(tok.type != EMPTY);

	path = getenv("PATH");		// get the PATH directories
	assert(strlen(path) < 1024);	// ensure our buffer is big enough

	// Copy PATH into pathbuf since strtok_r destroys the original string.
	// This allows us to pick apart the paths again for subsequent calls.
	// TODO: Try moving the parsing of all the PATH directories out so
	//	 this only occurs once.
	memcpy(pathbuf, path, 1024);
	curr = strtok_r(pathbuf, ":", &saveptr);

	// TODO: handle case user runs program relative to cwd
	// Look through each path to see if the program exists in one of them.
	while (curr != NULL) {
		char bin[PATH_MAX] = {0};
		size_t curr_len = strlen(curr);
		size_t bin_len = curr_len;

		// Copy the PATH directory bin path.
		memcpy(bin, curr, curr_len);

		// Check if the curr path has trailing '/' char.
		if (bin[bin_len-1] != '/') {
			bin[bin_len] = '/';
			bin_len += 1;
		}

		// Append the binary name to the path string
		assert(bin_len+tok.len < PATH_MAX);
		memcpy(&bin[bin_len], tok.raw, tok.len);
		bin_len += tok.len;

		struct stat file;
		int res = stat(bin, &file);

		// Open the program to read its output if it exists.
		// NOTE: Currently only opens the program in read mode.
		// NOTE: 1kb buffer size to read program output is small.
		if (res == 0) {
			// Piece together the rest of the tokens as arguments
			// to this program and pass them along.
			for (size_t i = 1; i < token_len; i++) {
				Token next = tokens[i];
				if (next.type == EMPTY)
					break;

				// Ensure there's enough space in the bin buffer.
				// +1 is added for the space to separate args.
				assert((bin_len+next.len+1) < PATH_MAX);

				// Add a space to separate this argument from
				// the last/program name and append it.
				bin[bin_len] = ' ';
				memcpy(&bin[bin_len+1], next.raw, next.len);
				bin_len += next.len + 1;
			}

			FILE *fp;
			fp = popen(bin, "r");

			if (fp != NULL) {
				char buf[1024];
				while (fgets(buf, sizeof(buf), fp) != NULL) {
					printf("%s", buf);
				}
				pclose(fp);
				return;
			}
		}
	
		curr = strtok_r(NULL, ":", &saveptr);
	}

	printf("\"%.*s\": No such program\n", (int)tok.len, tok.raw);
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
			#if DEBUG
				print_tokens(tokens, token_len);
			#endif
				execute_program(tokens, token_len);
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
