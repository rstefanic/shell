#include <assert.h>
#include <ctype.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "base.h"
#include "hashtable.h"
#include "lexer.h"
#include "log.h"
#include "memory.h"
#include "parser.h"
#include "string.h"

HashTable *symtable = {0};

typedef enum {
	BC_NONE,
	BC_CD,
	BC_PWD,
	BC_EXIT,
	BC_ECHO
} BuiltinCommand ;

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
		return BC_CD;
	} else if (compare_token("pwd", &tok)) {
		return BC_PWD;
	} else if (compare_token("exit", &tok)) {
		return BC_EXIT;
	} else if (compare_token("echo", &tok)) {
		return BC_ECHO;
	}

	return BC_NONE;
}

void interpolate_string(char* src, u64 srclen, char* dest, u64 destlen) {
	assert(srclen > 0);

	u64 src_i = 0;
	u64 dest_i = 0;
	while (src_i < srclen) {
		char c = src[src_i++];

		// If this is a dollar sign, then we're going read the value
		// as a variable to be interpolated.
		if (c == '$') {
			// Setup varname buffer to read the variable name.
			u64 maxvarnamelen = 256;
			char varnamebuf[maxvarnamelen];
			memset(varnamebuf, 0, maxvarnamelen);
			u64 j = 0;
			// Read the characters until we hit a non-alphanumeric.
			while (src_i < srclen) {
				c = src[src_i];
				if (!isalnum(c) || j >= maxvarnamelen)
					break;
				varnamebuf[j++] = c;
				src_i++;
			}

			// Get the variable from the symtable. If it's NULL,
			// fallback to see if it's an environment variable
			Entry *variable = hashtable_get(symtable, STR_LIT(varnamebuf));

			// TODO: Consolidate new symtable creation with the `~`
			//	 expansion from `handle_builtins`.
			if (variable == NULL) {
				const char *env_var_tmp = getenv(varnamebuf);
				assert(env_var_tmp != NULL);

				// Copy the value of the variable to the symtable.
				String *new_var = arena_alloc(symtable->arena, sizeof(String));
				char *env_var = arena_alloc(symtable->arena, strlen(env_var_tmp));
				strcpy(env_var, env_var_tmp);
				new_var->value = env_var;
				new_var->len = strlen(env_var);

				// Insert it and fetch it.
				hashtable_insert(symtable, STR_LIT(varnamebuf), new_var);
				variable = hashtable_get(symtable, STR_LIT(varnamebuf));
			}

			// If it's NULL at this point, then it's an error.
			// TODO: Report error instead of crashing program.
			if (variable == NULL) {
				printf("Unknown value \"%s\"", varnamebuf);
			}
			assert(variable != NULL);

			// Get the variable name from the environment and
			// replace the variable name in the destination string
			// with the evaluated name.
			String *var = (String*)variable->value;
			assert((dest_i+var->len) < destlen);

			j = 0;
			c = var->value[j];
			while (j < var->len) {
				dest[dest_i++] = c;
				c = var->value[++j];
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
	u64 child_idx = 1;
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
	case BC_CD: {
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
			u64 totallen = home.len;

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

		interpolate_string(path, strlen(path), final, PATH_MAX);

		int ok = chdir(final);
		if (ok != 0) {
			printf("cd: could not find directory \"%s\"\n", path);
		}
		break;
	}
	case BC_PWD: {
		printf("%s\n", path);
		break;
	}
	case BC_ECHO: {
		for (u64 i = 0; i < builtin_expression->data.list.length; i++) {
			Expression *next = builtin_expression->data.list.children[i];
			if (next == NULL)
				break;

			assert(next->type == EXPR_ATOM); // TODO: handle lists
			tok = &next->data.atom.value;

			char buf[1024] = {0};
			interpolate_string(tok->raw.value, tok->raw.len, buf, 1024);
			printf("%s ", buf);
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
void print_tokens(Token *tokens, u64 token_len) {
	printf("[DEBUG] TOKENS:\n");

	u64 i = 0;
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

void print_expression(Expression *expr, u64 indent);
void print_expressions(Expression *expressions, u64 expr_len, u64 indent) {
	if (indent == 0) {
		printf("[DEBUG] EXPRESSIONS:\n");
	}

	u64 i = 0;
	for (i = 0; i < expr_len; i++) {
		Expression expr = expressions[i];
		print_expression(&expr, indent);
	}
}

void tab(u64 n) {
	for (int i = 0; i < n; i++) 
		printf("\t");
}

void print_expression(Expression *expr, u64 indent) {
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
		u64 len = expr->data.list.length;
		for (u64 i = 0; i < len; i++) {
			Expression child = children[i];
			print_expression(&child, indent+1);
		}
	}
}
#endif

void execute_program(Expression *exec_node) {
	assert(exec_node->type == EXPR_LIST);
	u64 child_idx = 0;
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
		u64 curr_len = strlen(curr);
		u64 bin_len = curr_len;

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
	u64 child_idx = 0;
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
	if (curr == NULL) {
		return;
	}

	assert(curr->type == EXPR_ATOM);
	BuiltinCommand cmd = try_parse_builtin(curr);
	if (cmd == BC_NONE) {
		execute_program(expressions);
	} else if (cmd == BC_EXIT) {
		exit(0);
	} else {
		handle_builtin(expressions, cmd);
	}
}

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
		printf("\033[%lu;1H\033[K", min_row+1);
	}

	// Restore the old termios.
	tcsetattr(STDIN_FILENO, TCSANOW, &curr_termios);
}

int main() {
	unsigned char perm_arena_backing_buffer[MB(2)];
	Arena perm_arena = {0};
	arena_init(&perm_arena, perm_arena_backing_buffer, MB(2));
	assert(perm_arena.buf != NULL);

	// Configure global symbol table.
	Arena symtable_arena = {0};
	arena_init(&symtable_arena, arena_alloc(&perm_arena, KB(32)), KB(32));
	assert(symtable_arena.buf != NULL);
	symtable = hashtable_create(&symtable_arena);

	// Setup arena for the frame.
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

	#if DEBUG
		print_tokens(tokens, token_len);
	#endif

		u64 expressions_len = 128;
		Expression *expressions = arena_alloc(&frame_arena, expressions_len * sizeof(Expression));
		assert(expressions != NULL);
		parse(expressions, expressions_len, tokens, token_len);
	#if DEBUG
		print_expressions(expressions, expressions_len, 0);
	#endif

		Expression *expr = &expressions[0];
		assert(expr->type == EXPR_LIST);
		eval_expressions(expr);

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
