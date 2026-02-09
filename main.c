#include <assert.h>
#include <ctype.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hashtable.h"
#include "lexer.h"
#include "memory.h"
#include "parser.h"
#include "string.h"

#define ARENA_BYTES_LEN 1024 * 128 // 128kb
unsigned char arena_backing_buffer[ARENA_BYTES_LEN];
Arena arena = {0};

#define SYMTABLE_ARENA_BYTES_LEN 1024 * 32 // 32kb
unsigned char symtable_backing_buffer[SYMTABLE_ARENA_BYTES_LEN];
Arena symtable_arena = {0};
HashTable *symtable = {0};

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

BuiltinCommand try_parse_builtin(Expression *expr) {
	assert(expr->type == EXPR_ATOM);
	Token tok = expr->data.atom.value;

	char buf[256];
	assert(tok.raw.len < 256);
	memcpy(buf, tok.raw.value, tok.raw.len);

	if (compare_token("cd", &tok)) {
		return CD;
	} else if (compare_token("pwd", &tok)) {
		return PWD;
	} else if (compare_token("exit", &tok)) {
		return EXIT;
	} else if (compare_token("echo", &tok)) {
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
			// Setup varname buffer
			size_t maxvarnamelen = 256;
			char varnamebuf[maxvarnamelen];
			memset(varnamebuf, 0, maxvarnamelen);
			size_t j = 0;

			// Read the characters until we hit a non-alphanumeric.
			while (src_i < srclen) {
				c = src[src_i];
				if (!isalnum(c) || j >= maxvarnamelen)
					break;
				varnamebuf[j++] = c;
				src_i++;
			}

			// Get the variable name from the environment and
			// replace the variable name in the destination string
			// with the evaluated name.
			char* var = getenv(varnamebuf);
			if (var != NULL) {
				assert(dest_i + strlen(var) < destlen);
				j = 0;
				c = var[j];
				while (c != '\0') {
					dest[dest_i++] = c;
					c = var[++j];
				}
			}
		} else {
			// Otherwise we'll simply copy the char to the dest str.
			if (dest_i < destlen) { // check if we have enough space.
				dest[dest_i++] = c;
			}
		}
	}
}

void handle_builtin(Expression *builtin_expression, BuiltinCommand type) {
	assert(builtin_expression->type == EXPR_LIST);

	Token *tok = NULL;

	// Skip the first ATOM since we know what it is by the `type` parameter.
	size_t child_idx = 1;
	if (builtin_expression->data.list.length > 1) {
		Expression *curr = builtin_expression->data.list.children[child_idx];
		tok = &curr->data.atom.value;
	}

	// Pull out the current working directory and store it in path. Some
	// builtin commands need to manipulate the path so setting this to the
	// current working directory is a sensible default.
	char path[PATH_MAX] = {0};
	char final[PATH_MAX] = {0};
	char *res = getcwd(path, PATH_MAX);
	assert(res != NULL);

	switch(type) {
	case CD: {
		if (tok->type == TOK_EOF) {
			// If there is no argument to CD, then send them HOME.
			tok->type = TOK_IDENT;
			tok->raw.value = "~";
			tok->raw.len = 1;
		}

		// Set the path to the directory specified by the user if it's
		// an absolute path. Zero out the remaining contents of the path
		// buffer so that we avoid conflicts with the user's directory.
		if (tok->raw.value[0] == '/' || tok->raw.value[0] == '$') {
			memcpy(path, tok->raw.value, tok->raw.len);
			memset(path+tok->raw.len, 0, PATH_MAX-tok->raw.len);
		} else if (tok->raw.value[0] == '~') {
			Entry *home_symbol = hashtable_get(symtable, STR_LIT("HOME"));
			// If `$HOME` is NULL, first see if we can fetch in from env.
			if (home_symbol == NULL) {
				const char* env_home_tmp = getenv("HOME");
				assert(env_home_tmp != NULL);

				// Copy the value of the $HOME to the symtable.
				String *home_value = arena_alloc(symtable->arena, sizeof(String));
				char* env_home = arena_alloc(symtable->arena, strlen(env_home_tmp));
				strcpy(env_home, env_home_tmp);
				home_value->value = env_home;
				home_value->len = strlen(env_home);

				// Insert it and fetch it. If HOME is null
				// again, then crash because we can't go `~`.
				hashtable_insert(symtable, STR_LIT("HOME"), home_value);
				home_symbol = hashtable_get(symtable, STR_LIT("HOME"));
				assert(home_symbol != NULL);
			}

			String home = *((String*)home_symbol->value);
			size_t totallen = home.len;

			memcpy(path, home.value, home.len);

			if (tok->raw.len > 1) {
				memcpy(&path[home.len], &tok->raw.value[1], tok->raw.len - 1);
				totallen += tok->raw.len - 1;
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

			assert((pathlen+tok->raw.len) < PATH_MAX);
			memcpy(&path[pathlen], tok->raw.value, tok->raw.len);
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
		while (i+1 < builtin_expression->data.list.length) {
			char buf[1024] = {0};
			eval_env_variables(tok->raw.value, tok->raw.len, buf, 1024);
			printf("%s ", buf);
			Expression *next = builtin_expression->data.list.children[++i];
			assert(next->type == EXPR_ATOM); // TODO: handle lists
			tok = &next->data.atom.value;
		}

		// Ending newline for the prompt to start on the next line.
		printf("\n");
		break;
	}
	default:
		assert(false); // unreachable
	}
}

#if DEBUG
void print_tokens(Token *tokens, size_t token_len) {
	printf("[DEBUG] TOKENS:\n");

	size_t i = 0;
	for (i = 0; i < token_len; i++) {
		Token tok = tokens[i];

		if (tok.type == TOK_EOF)
			break;

		printf("type: %s, raw: [%.*s]\n",
			debug_token_type_to_string(tok.type),
			(int)tok.raw.len,
			tok.raw.value);
	}

	printf("\n\n");
}

void print_expression(Expression *expr, size_t indent);
void print_expressions(Expression *expressions, size_t expr_len, size_t indent) {
	if (indent == 0) {
		printf("[DEBUG] EXPRESSIONS:\n");
	}

	size_t i = 0;
	for (i = 0; i < expr_len; i++) {
		Expression expr = expressions[i];
		print_expression(&expr, indent);
	}
}

void tab(size_t n) {
	for (int i = 0; i < n; i++) 
		printf("\t");
}

void print_expression(Expression *expr, size_t indent) {
	tab(indent);
	if (expr->type == EXPR_ATOM) {
		printf("type: EXPR_ATOM, ");
		tab(indent);
		switch (expr->data.atom.kind) {
		case ATOM_IDENT:
			printf("kind: ATOM_IDENT, ");
			break;
		case ATOM_NUMBER:
			printf("kind: ATOM_NUMBER, ");
			break;
		case ATOM_STRING:
			printf("kind: ATOM_STRING, ");
			break;
		}

		Token tok = expr->data.atom.value;
		printf("raw: [%.*s]\n", (int)tok.raw.len, tok.raw.value);
	} else {
		Expression *children = *(expr->data.list.children);
		size_t len = expr->data.list.length;
		for (size_t i = 0; i < len; i++) {
			Expression child = children[i];
			print_expression(&child, indent+1);
		}
	}
}
#endif

void execute_program(Expression *exec_node) {
	assert(exec_node->type == EXPR_LIST);
	size_t child_idx = 0;
	Expression *child = exec_node->data.list.children[child_idx];

	// TODO: Handle recursive nodes if child itself is a EXPR_LIST
	assert(child->type == EXPR_ATOM);
	Token tok = child->data.atom.value;
	char pathbuf[1024];	// copy of the PATH environment variable
	char *path;		// for the PATH environment variable
	char *saveptr;		// to maintain context between strtok_r calls
	char *curr;		// current path that's being checked

	// Make sure this token isn't empty.
	assert(tok.type != TOK_EOF);

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
			child_idx += 1;
			Expression *curr = exec_node->data.list.children[child_idx];

			// Piece together the rest of the tokens as arguments
			// to this program and pass them along.
			while (curr != NULL) {
				// TODO: Handle evaluating EXPR_LIST in children
				assert(curr->type == EXPR_ATOM);
				Token next = curr->data.atom.value;
				if (next.type == TOK_EOF)
					break;

				// Ensure there's enough space in the bin buffer.
				// +1 is added for the space to separate args.
				assert((bin_len+next.raw.len+1) < PATH_MAX);

				// Add a space to separate this argument from
				// the last/program name and append it.
				bin[bin_len] = ' ';
				memcpy(&bin[bin_len+1], next.raw.value, next.raw.len);
				bin_len += next.raw.len + 1;

				// Advance the node
				child_idx += 1;
				assert(child_idx <= exec_node->data.list.length);
				curr = exec_node->data.list.children[child_idx];
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

void eval_expressions(Expression *expressions) {
	assert(expressions->type == EXPR_LIST);
	Expression **children = expressions->data.list.children;
	size_t child_idx = 0;
	Expression *curr = children[child_idx];

	// Evaluate any sub expressions
	while (curr != NULL && child_idx < expressions->data.list.length) {
		// TODO: eval_expression will need to change to replace the
		// EXPR_LIST node here and overwrite it. This means we will
		// probably have to create a more complicated run time env.
		if (curr->type == EXPR_LIST) {
			eval_expressions(curr);
		}
		child_idx += 1;
		curr = children[child_idx];
	}

	// Now evaluate this list as a whole
	curr = children[0];
	assert(curr != NULL);
	assert(curr->type == EXPR_ATOM);
	BuiltinCommand cmd = try_parse_builtin(curr);
	if (cmd == NONE) {
		execute_program(expressions);
	} else if (cmd == EXIT) {
		exit(0);
	} else {
		handle_builtin(expressions, cmd);
	}
}

int main() {
	arena_init(&arena, arena_backing_buffer, ARENA_BYTES_LEN);
	arena_init(&symtable_arena, symtable_backing_buffer, SYMTABLE_ARENA_BYTES_LEN);
	symtable = hashtable_create(&symtable_arena);

	for(;;) {
		arena_free(&arena);
		String input = string_new(&arena, 256);

		printf("> ");
		fgets(input.value, input.len, stdin);

		size_t token_len = 256;
		Token *tokens = arena_alloc(&arena, token_len * sizeof(Token));
		assert(tokens != NULL);
		lex(tokens, token_len, &input);
	#if DEBUG
		print_tokens(tokens, token_len);
	#endif

		size_t expressions_len = 128;
		Expression *expressions = arena_alloc(&arena, expressions_len * sizeof(Expression));
		assert(expressions != NULL);
		parse(expressions, expressions_len, tokens, token_len);
	#if DEBUG
		print_expressions(expressions, expressions_len, 0);
	#endif

		Expression *expr = &expressions[0];
		assert(expr->type == EXPR_LIST);
		eval_expressions(expr);
	}

	return 0;
}
