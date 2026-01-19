#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#include "lexer.h"

bool end(Lexer *lexer) {
	char c = lexer->buf[lexer->ptr];
	return c == '\0';
}

char peek(Lexer *lexer) {
	return (char)lexer->buf[lexer->ptr];
}

char peek_next(Lexer *lexer) {
	uintptr_t ptr = lexer->ptr+1;
	assert(ptr <= lexer->len);
	return (char)lexer->buf[ptr];
}

void advance(Lexer *lexer) {
	uintptr_t new_ptr = lexer->ptr+1;
	assert(new_ptr <= lexer->len); // TODO: revisit assertion
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
	char *start = &lexer->buf[lexer->ptr];
	size_t len = 0;

	while (!end(lexer)) {
		char c = peek(lexer);
		if (is_whitespace(c))
			break;

		advance(lexer);
		len += 1;
	}

	tok->raw = start;
	tok->len = len;
	tok->type = LITERAL;
}

void lex(Token* tokens, size_t token_len, char *input, size_t input_len) {
	Lexer lexer = {
		.buf = input,
		.len = input_len,
		.ptr = 0,
	};

	size_t i = 0; // iterator for available tokens
	while (!end(&lexer)) {
		assert(i < token_len); // ensure we have space to parse this
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

