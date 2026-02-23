#include <ctype.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "evaluator.h"
#include "parser.h"

Value NIL_VALUE = {
	.type = VAL_NIL
};

Runtime create_runtime(Arena *arena) {
	HashTable *symtable = hashtable_create(arena);
	StackFrame *stack_frame = arena_alloc(arena, sizeof(StackFrame));
	return (Runtime) {
		.arena = arena,
		.symtable = symtable,
		.stack_frame = stack_frame
	};
}

bool compare_token(char* value, Token *tok) {
	if (strlen(value) != tok->raw.len) {
		return false;
	}

	return memcmp(tok->raw.value, value, tok->raw.len) == 0;
}

BuiltinCommand try_parse_builtin(struct Atom *atom) {
	String *value = &atom->value.raw;

	if (string_compare(&STR_LIT("cd"), value)) {
		return BC_CD;
	} else if (string_compare(&STR_LIT("pwd"), value)) {
		return BC_PWD;
	} else if (string_compare(&STR_LIT("exit"), value)) {
		return BC_EXIT;
	} else if (string_compare(&STR_LIT("echo"), value)) {
		return BC_ECHO;
	}

	return BC_NONE;
}

bool is_executable_ident(struct Atom *atom) {
	assert(atom->kind == ATOM_IDENT);
	String raw = atom->value.raw;
	return raw.len > 0 && raw.value[0] == '!';
}

Value atom_to_value(Runtime *runtime, struct Atom *atom) {
	switch (atom->kind) {
	case ATOM_NUMBER: {
		// TODO: Use strol over atoi
		i64 out = atoi(atom->value.raw.value);
		return (Value) {
			.type = VAL_NUMBER,
			.data.number = out
		};
	}
	case ATOM_STRING:
		return (Value) {
			.type = VAL_STRING,
			.data.string = &atom->value.raw
		};
	case ATOM_IDENT: {
		String ident_raw = atom->value.raw;

		// Check if it has the executable prefix of '!'.
		if (is_executable_ident(atom)) {
			return (Value) {
				.type = VAL_EXECUTABLE,
				.data.string = &atom->value.raw,
			};
		}

		// Check if it's a built in command we can evaluate.
		BuiltinCommand cmd = try_parse_builtin(atom);
		if (cmd == BC_EXIT) {
			exit(0);
		} else if (cmd != BC_NONE) {
			return (Value) {
				.type = VAL_BUILTIN,
				.data.cmd = cmd
			};
		}

		// In all other cases, we'll try and resolve it as a symtable variable.
		Entry *variable = hashtable_get(runtime->symtable, ident_raw);

		// If upon the first attempt the value doesn't exist, try to
		// see if the value is found in the environment variables.
		// TODO: Move into new function.
		if (variable == NULL) {
			u64 maxvarnamelen = 256;
			char varnamebuf[maxvarnamelen];
			memset(varnamebuf, 0, maxvarnamelen);
			memcpy(varnamebuf, ident_raw.value, maxvarnamelen);

			printf("varnamebuf: %s\n", varnamebuf);
			const char *env_var_tmp = getenv(varnamebuf);
			assert(env_var_tmp != NULL);

			// Copy the value of the variable to the symtable.
			String *new_var = arena_alloc(runtime->symtable->arena, sizeof(String));
			char *env_var = arena_alloc(runtime->symtable->arena, strlen(env_var_tmp));
			strcpy(env_var, env_var_tmp);
			new_var->value = env_var;
			new_var->len = strlen(env_var);

			// Insert it and fetch it.
			hashtable_insert(runtime->symtable, STR_LIT(varnamebuf), new_var);
			variable = hashtable_get(runtime->symtable, STR_LIT(varnamebuf));
		}

		// If we still don't know what the value is here, then it's an error.
		if (variable == NULL) {
			printf("Unknown value \"%.*s\n", (u32)ident_raw.len, ident_raw.value);
		}
		assert(variable != NULL);
		return (Value) {
			.type = VAL_STRING,
			.data.string = &atom->value.raw,
		};
	}
	}

	return NIL_VALUE;
}

Value evaluate_stack_frame(Runtime *runtime) {
	StackFrame *stack_frame = runtime->stack_frame;
	assert(stack_frame != NULL);

	Expression *expr = stack_frame->expression;
	switch (expr->type) {
	case EXPR_ATOM:
		return atom_to_value(runtime, &expr->data.atom);
	default: {
		u64 list_count = expr->data.list.size;
		stack_frame->values = arena_alloc(runtime->arena, sizeof(Value) * list_count);
		stack_frame->values_len = expr->data.list.size;

		for (u64 i = 0; i < stack_frame->values_len; i++) {
			Expression *curr = &expr->data.list.expressions[i];
			if (curr->type == EXPR_ATOM) {
				stack_frame->values[i] = atom_to_value(runtime, &curr->data.atom);
			} else {
				StackFrame *inner_stack_frame = arena_alloc(runtime->arena, sizeof(StackFrame));
				inner_stack_frame->prev_frame = stack_frame;
				inner_stack_frame->expression = curr->data.list.expressions;

				// Set the new stackframe for the runtime and re-run
				runtime->stack_frame = inner_stack_frame;
				stack_frame->values[i] = evaluate_stack_frame(runtime);

				// Restore the old stack frame
				runtime->stack_frame = stack_frame;
			}
		}


		// Now evaluate the list.
		assert(stack_frame->values != NULL);
		Value initial_value = stack_frame->values[0];

		switch (initial_value.type) {
		case VAL_BUILTIN:
			return handle_builtin(runtime, stack_frame);
		case VAL_EXECUTABLE:
			return execute_program(runtime);
		default:
			return NIL_VALUE;
		}
	}
	};
}

Value *evaluator_eval_expressions(Runtime *runtime, Expression *expr) {
	Value *out = &NIL_VALUE;
	runtime->stack_frame->expression = expr;
	evaluate_stack_frame(runtime);
	return out;
}

Value handle_builtin(Runtime *runtime, StackFrame *stack_frame) {
	assert(stack_frame->values != NULL);
	assert(stack_frame->values[0].type == VAL_BUILTIN);
	Value *values = stack_frame->values;
	BuiltinCommand builtin_cmd = values->data.cmd;

	// Pull out the current working directory and store it in path. Some
	// builtin commands need to manipulate the path so setting this to the
	// current working directory is a sensible default.
	char path[PATH_MAX] = {0};
	char final[PATH_MAX] = {0};
	char *res = getcwd(path, PATH_MAX);
	assert(res != NULL);

	switch(builtin_cmd) {
	case BC_CD: {
		// Default the user to their HOME directory.
		Value destination = (Value) {
			.type = VAL_STRING,
			.data.string = &STR_LIT("~")
		};

		// Use the next value as the argument to CD if it exists.
		if (stack_frame->values_len > 1) {
			destination = stack_frame->values[1];
		}

		// Set the path to the directory specified by the user if it's
		// an absolute path. Zero out the remaining contents of the path
		// buffer so that we avoid conflicts with the user's directory.
		if (
			destination.data.string->value[0] == '/' ||
			destination.data.string->value[0] == '$'
		) {
			memcpy(
				path,
				destination.data.string->value,
				destination.data.string->len
			);

			u64 dest_len = destination.data.string->len;
			memset(path+dest_len, 0, PATH_MAX-dest_len);
		} else if (destination.data.string->value[0] == '~') {
			Entry *home_symbol = hashtable_get(runtime->symtable, STR_LIT("HOME"));
			// If `$HOME` is NULL, first see if we can fetch in from env.
			if (home_symbol == NULL) {
				const char* env_home_tmp = getenv("HOME");
				assert(env_home_tmp != NULL);

				// Copy the value of the $HOME to the symtable.
				String *home_value = arena_alloc(runtime->symtable->arena, sizeof(String));
				char* env_home = arena_alloc(runtime->symtable->arena, strlen(env_home_tmp));
				strcpy(env_home, env_home_tmp);
				home_value->value = env_home;
				home_value->len = strlen(env_home);

				// Insert it and fetch it. If HOME is null
				// again, then crash because we can't go `~`.
				hashtable_insert(runtime->symtable, STR_LIT("HOME"), home_value);
				home_symbol = hashtable_get(runtime->symtable, STR_LIT("HOME"));
				assert(home_symbol != NULL);
			}

			String home = *((String*)home_symbol->value);
			u64 totallen = home.len;

			memcpy(path, home.value, home.len);

			if (destination.data.string->len > 1) {
				memcpy(&path[home.len], &destination.data.string->value[1], destination.data.string->len - 1);
				totallen += destination.data.string->len - 1;
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

			assert((pathlen+destination.data.string->len) < PATH_MAX);
			memcpy(&path[pathlen], destination.data.string->value, destination.data.string->len);
		}

		interpolate_string(runtime, path, strlen(path), final, PATH_MAX);

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
		for (u64 i = 1; i < stack_frame->values_len; i++) {
			Value *next = stack_frame->values;
			if (next == NULL)
				break;

			char buf[1024] = {0};
			interpolate_string(
				runtime,
				next->data.string->value,
				next->data.string->len,
				buf,
				1024
			);
			printf("%s ", buf);
		}

		// Ending newline for the prompt to start on the next line.
		printf("\n");
		break;
	}
	default:
		assert(false); // unreachable
	}

	return NIL_VALUE;
}

void interpolate_string(Runtime *runtime, char* src, u64 srclen, char* dest, u64 destlen) {
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
			Entry *variable = hashtable_get(runtime->symtable, STR_LIT(varnamebuf));

			// TODO: Consolidate new symtable creation with the `~`
			//	 expansion from `handle_builtins`.
			if (variable == NULL) {
				const char *env_var_tmp = getenv(varnamebuf);
				assert(env_var_tmp != NULL);

				// Copy the value of the variable to the symtable.
				String *new_var = arena_alloc(runtime->symtable->arena, sizeof(String));
				char *env_var = arena_alloc(runtime->symtable->arena, strlen(env_var_tmp));
				strcpy(env_var, env_var_tmp);
				new_var->value = env_var;
				new_var->len = strlen(env_var);

				// Insert it and fetch it.
				hashtable_insert(runtime->symtable, STR_LIT(varnamebuf), new_var);
				variable = hashtable_get(runtime->symtable, STR_LIT(varnamebuf));
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

Value execute_program(Runtime *runtime) {
	assert(runtime->stack_frame != NULL);
	StackFrame *stack_frame = runtime->stack_frame;

	assert(stack_frame->values != NULL);
	assert(stack_frame->values[0].type == VAL_EXECUTABLE);

	Value *values = stack_frame->values;
	String *exec_name = values[0].data.symbol;

	char pathbuf[1024];	// copy of the PATH environment variable
	char *path;		// for the PATH environment variable
	char *saveptr;		// to maintain context between strtok_r calls
	char *curr;		// current path that's being checked

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
		assert(bin_len+exec_name->len < PATH_MAX);
		memcpy(&bin[bin_len], exec_name->value, exec_name->len);
		bin_len += exec_name->len;

		struct stat file;
		int res = stat(bin, &file);

		// Open the program to read its output if it exists.
		// NOTE: Currently only opens the program in read mode.
		// NOTE: 1kb buffer size to read program output is small.
		if (res == 0) {
			// Append the rest of the values as arguments to the program.
			for (u64 i = 1; i < stack_frame->values_len; i++) {
				Value *curr = &values[i];
				assert(curr->type == VAL_STRING); // TODO: Allow other types

				// Ensure there's enough space in the bin buffer.
				// +1 is added for the space to separate args.
				assert((bin_len+curr->data.string->len+1) < PATH_MAX);

				// Add a space to separate this argument from
				// the last/program name and append it.
				bin[bin_len] = ' ';
				memcpy(&bin[bin_len+1], curr->data.string->value, curr->data.string->len);
				bin_len += curr->data.string->len + 1;
			}

			FILE *fp;
			fp = popen(bin, "r");

			if (fp != NULL) {
				char buf[1024];
				while (fgets(buf, sizeof(buf), fp) != NULL) {
					printf("%s", buf);
				}
				pclose(fp);
				return NIL_VALUE;
			}
		}
	
		curr = strtok_r(NULL, ":", &saveptr);
	}

	printf("\"%.*s\": No such program\n", (int)exec_name->len, exec_name->value);
	return NIL_VALUE;
}
