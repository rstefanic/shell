#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "log.h"

bool end(Lexer *lexer) {
	char c = lexer->buf->value[lexer->ptr];
	return c == '\0';
}

char peek(Lexer *lexer) {
	return (char)lexer->buf->value[lexer->ptr];
}

char peek_next(Lexer *lexer) {
	uintptr_t ptr = lexer->ptr+1;
	assert(ptr <= lexer->buf->len);
	return (char)lexer->buf->value[ptr];
}

void advance(Lexer *lexer) {
	uintptr_t new_ptr = lexer->ptr+1;
	assert(new_ptr <= lexer->buf->len); // TODO: revisit assertion
	lexer->ptr = new_ptr;
}

bool is_whitespace(char c) {
	switch(c) {
	case ' ':
	case '\t':
	case '\n':
		return true;
	default:
		return false;
	};
}

void parse_whitespace(Lexer *lexer) {
	while (!end(lexer)) {
		char c = peek(lexer);
		if (is_whitespace(c)) {
			advance(lexer);
			continue;
		}

		break;
	}
}

void parse_paren(Lexer *lexer, Token *tok) {
	char c = peek(lexer);
	tok->raw = string_slice(lexer->buf, lexer->ptr, 1);
	tok->type = c == '(' ? TOK_LEFTPAREN : TOK_RIGHTPAREN;
	advance(lexer);
}

void parse_string(Lexer *lexer, Token *tok) {
	u64 start = lexer->ptr;
	u64 len = 0;
	char prev = '\0'; // tracks the previous character to check escape seq

	while (!end(lexer)) {
		char c = peek(lexer);

		// Check if this double quote character was escaped or not.
		bool done = c == '\"' && prev != '\\' && len > 0;

		prev = c;
		advance(lexer);
		len += 1;

		if (done)
			break;
	}

	tok->raw = string_slice(lexer->buf, start, len);
	tok->type = TOK_STRING;
}

void parse_number(Lexer *lexer, Token *tok) {
	u64 start = lexer->ptr;
	u64 len = 0;
	while (!end(lexer)) {
		char c = peek(lexer);
		if (!(isdigit(c) || c == '.'))
			break;

		advance(lexer);
		len += 1;
	}

	tok->raw = string_slice(lexer->buf, start, len);
	tok->type = TOK_NUMBER;
}

// Identifiers can start with any a letter or any of the following chars.
bool is_ident_prefix_char(char c) {
	return isalpha(c)
		|| c == '\''
		|| c == '$'
		|| c == '!'
		|| c == '-'
		|| c == '~'
		|| c == '/';
}

// The remainder of an identifier can be made up of letters, numbers, or the
// following characters.
bool is_ident_char(char c) {
	return isalnum(c)
		|| c == '?' 
		|| c == '/'
		|| c == '-'
		|| c == '~';
}

void parse_identifier(Lexer *lexer, Token *tok) {
	u64 start = lexer->ptr;
	u64 len = 0;
	char c = peek(lexer);

	// Assume that this code path only runs when the caller knows that
	// the first character is one of the starting identifier characters.
	assert(is_ident_prefix_char(c));
	advance(lexer);
	len += 1;

	while (!end(lexer)) {
		c = peek(lexer);
		if (!is_ident_char(c))
			break;

		advance(lexer);
		len += 1;
	}

	tok->raw = string_slice(lexer->buf, start, len);
	tok->type = TOK_IDENT;
}

bool lex(Token* tokens, u64 token_len, String *input) {
	Lexer lexer = {
		.buf = input,
		.ptr = 0,
	};

	u64 i = 0; // iterator for available tokens
	while (!end(&lexer)) {
		// Assert that we have enough space in our Token buffer to parse.
		assert(i < token_len);
		Token *tok = &tokens[i];

		char c = peek(&lexer);
		if (is_whitespace(c)) {
			parse_whitespace(&lexer);
			continue;
		}

		if (c == '(' || c == ')')
			parse_paren(&lexer, tok);
		else if (c == '\"')
			parse_string(&lexer, tok);
		else if (isdigit(c))
			parse_number(&lexer, tok);
		else if (is_ident_prefix_char(c))
			parse_identifier(&lexer, tok);
		else {
			printf("Unrecognized token: \"%c\"\n", c);
			return false;
		}

		i += 1;
	}

	char buf[256] = { 0 };
	snprintf(buf, 256, "(lexer) lex memory used: %llu bytes\n", sizeof(Token) * i);
	log_emit_message(LOG_LEVEL_INFO, (String) {
		.value = buf,
		.len = strlen(buf)
	});

	return true;
}
