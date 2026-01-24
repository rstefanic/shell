#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#include "lexer.h"

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

void parse_literal(Lexer *lexer, Token *tok) {
	size_t start = lexer->ptr;
	size_t len = 0;
	while (!end(lexer)) {
		char c = peek(lexer);
		if (is_whitespace(c))
			break;

		advance(lexer);
		len += 1;
	}

	tok->raw = string_slice(lexer->buf, start, len);
	tok->type = LITERAL;
}

void lex(Token* tokens, size_t token_len, String *input) {
	Lexer lexer = {
		.buf = input,
		.ptr = 0,
	};

	size_t i = 0; // iterator for available tokens
	while (!end(&lexer)) {
		// Assert that we have enough space in our Token buffer to parse.
		assert(i < token_len);
		Token *tok = &tokens[i];

		char c = peek(&lexer);
		if (is_whitespace(c)) {
			parse_whitespace(&lexer);
		} else {
			parse_literal(&lexer, tok);
			i += 1;
		}
	}
}

