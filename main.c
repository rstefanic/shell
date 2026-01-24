#include <assert.h>
#include <ctype.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lexer.h"
#include "memory.h"
#include "string.h"

typedef enum BuiltinCommand BuiltinCommand;
enum BuiltinCommand {
	NONE,
	CD,
	PWD,
	EXIT,
	ECHO
};

bool compare_token(char* value, Token *tok) {
	if (strlen(value) != tok->raw.len) {
		return false;
	}

	return memcmp(tok->raw.value, value, tok->raw.len) == 0;
}

BuiltinCommand try_parse_builtin(Token *tok) {
	char buf[256];
	assert(tok->raw.len < 256);
	memcpy(buf, tok->raw.value, tok->raw.len);

	if (compare_token("cd", tok)) {
		return CD;
	} else if (compare_token("pwd", tok)) {
		return PWD;
	} else if (compare_token("exit", tok)) {
		return EXIT;
	} else if (compare_token("echo", tok)) {
		return ECHO;
	}

	return NONE;
}

void eval_env_variables(char* src, size_t srclen, char* dest, size_t destlen) {
	assert(srclen > 0);

	size_t src_i = 0;
	size_t dest_i = 0;
	while (src_i < srclen) {
		char c = src[src_i++];

		// If this is a dollar sign, then we're going to enter into
		// reading this as a variable to be interpreted.
		if (c == '$') {
			c = src[src_i++]; // advance one to skip the '$'

			// Setup varname buffer
			size_t maxvarnamelen = 256;
			char varnamebuf[maxvarnamelen];
			memset(varnamebuf, 0, maxvarnamelen);
			size_t j = 0;

			// Read the characters until we hit a non-alphanumeric.
			while (c != '\0' && isalnum(c) && j < maxvarnamelen) {
				varnamebuf[j++] = c;
				c = src[src_i++];
			}

			// Back up the pointer one to "unconsume" the last
			// character that was not part of the variable.
			src_i--;

			// Get the variable name from the environment and
			// replace the variable name in the destination string
			// with the evaluated name.
			char* var = getenv(varnamebuf);
			if (var != NULL) {
				j = 0;
				c = var[j];
				while (c != '\0') {
					dest[dest_i++] = c;
					c = var[++j];
				}
			}
		} else {
			// Otherwise we'll simply copy the char to the dest str.
			dest[dest_i++] = c;
		}
	}
}

void handle_builtin(Token *tokens, BuiltinCommand type) {
	Token tok = tokens[0];

	// Pull out the current working directory and store it in path. Some
	// builtin commands need to manipulate the path so setting this to the
	// current working directory is a sensible default.
	char path[PATH_MAX] = {0};
	char final[PATH_MAX] = {0};
	char *res = getcwd(path, PATH_MAX);
	assert(res != NULL);

	switch(type) {
	case CD: {
		if (tok.type == EMPTY) {
			// If there is no argument to CD, then send them HOME.
			tok.type = LITERAL;
			tok.raw.value = "~";
			tok.raw.len = 1;
		}

		// Set the path to the directory specified by the user if it's
		// an absolute path. Zero out the remaining contents of the path
		// buffer so that we avoid conflicts with the user's directory.
		if (tok.raw.value[0] == '/' || tok.raw.value[0] == '$') {
			memcpy(path, tok.raw.value, tok.raw.len);
			memset(path+tok.raw.len, 0, PATH_MAX-tok.raw.len);
		} else if (tok.raw.value[0] == '~') {
			const char* home = getenv("HOME");
			size_t homelen = strlen(home);
			size_t totallen = homelen;
			memcpy(path, home, homelen);

			if (tok.raw.len > 1) {
				memcpy(&path[homelen], &tok.raw.value[1], tok.raw.len - 1);
				totallen += tok.raw.len - 1;
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

			assert((pathlen+tok.raw.len) < PATH_MAX);
			memcpy(&path[pathlen], tok.raw.value, tok.raw.len);
		}

		eval_env_variables(path, strlen(path), final, PATH_MAX);

		int ok = chdir(final);
		if (ok != 0) {
			printf("cd: could not find directory \"%s\"\n", path);
		}
		break;
	}
	case PWD: {
		printf("%s\n", path);
		break;
	}
	case ECHO: {
		size_t i = 0; // *tokens start at 0

		// TODO: Better bounds handling of the token sizes
		while (tok.type != EMPTY && i < 255) {
			char buf[1024] = {0};
			eval_env_variables(tok.raw.value, tok.raw.len, buf, 1024);
			printf("%s ", buf);
			tok = tokens[++i];
		}

		// Ending newline for the prompt to start on the next line.
		printf("\n");
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
			(int)tok.raw.len,
			tok.raw.value);
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
	assert(path != NULL);
	assert(strlen(path) < 1024);	// ensure our buffer is big enough

	// Copy PATH into pathbuf since strtok_r destroys the original string.
	// This allows us to pick apart the paths again for subsequent calls.
	// TODO: Try moving the parsing of all the PATH directories out so
	//	 this only occurs once.
	strcpy(pathbuf, path);
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
		assert(bin_len+tok.raw.len < PATH_MAX);
		memcpy(&bin[bin_len], tok.raw.value, tok.raw.len);
		bin_len += tok.raw.len;

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
				assert((bin_len+next.raw.len+1) < PATH_MAX);

				// Add a space to separate this argument from
				// the last/program name and append it.
				bin[bin_len] = ' ';
				memcpy(&bin[bin_len+1], next.raw.value, next.raw.len);
				bin_len += next.raw.len + 1;
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

	printf("\"%.*s\": No such program\n", (int)tok.raw.len, tok.raw.value);
}

int main() {
	size_t backing_buffer_len = 1024 * 10; // 10kb
	unsigned char backing_buffer[backing_buffer_len];

	Arena a = {0};
	arena_init(&a, backing_buffer, backing_buffer_len);

	for(;;) {
		arena_free(&a);
		String input = string_new(&a, 256);

		printf("> ");
		fgets(input.value, input.len, stdin);

		size_t token_len = 256;
		Token *tokens = arena_alloc(&a, token_len * sizeof(Token));
		lex(tokens, token_len, &input);

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
