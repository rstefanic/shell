#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

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

void parse_paren(Lexer *lexer, Token *tok) {
	char c = peek(lexer);
	tok->raw = string_slice(lexer->buf, lexer->ptr, 1);
	tok->type = c == '(' ? TOK_LEFTPAREN : TOK_RIGHTPAREN;
	advance(lexer);
}

void parse_string(Lexer *lexer, Token *tok) {
	size_t start = lexer->ptr;
	size_t len = 0;
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
	size_t start = lexer->ptr;
	size_t len = 0;
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

void parse_identifier(Lexer *lexer, Token *tok) {
	size_t start = lexer->ptr;
	size_t len = 0;
	while (!end(lexer)) {
		char c = peek(lexer);
		if (!isalnum(c) && c != '?' && c != '$' && c != '/' && c != '-')
			break;

		advance(lexer);
		len += 1;
	}

	tok->raw = string_slice(lexer->buf, start, len);
	tok->type = TOK_IDENT;
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
			continue;
		}

		if (c == '(' || c == ')') {
			parse_paren(&lexer, tok);
		} else if (c == '\"') {
			parse_string(&lexer, tok);
		} else if (isdigit(c)) {
			parse_number(&lexer, tok);
		} else {
			parse_identifier(&lexer, tok);
		}

		i += 1;
	}
}

#ifdef DEBUG
const char* debug_token_type_to_string(TokenType type) {
	switch (type) {
	case TOK_EOF:
		return "TOK_EOF";
	case TOK_LEFTPAREN:
		return "TOK_LEFTPAREN";
	case TOK_RIGHTPAREN:
		return "TOK_RIGHTPAREN";
	case TOK_IDENT:
		return "TOK_IDENT";
	case TOK_NUMBER:
		return "TOK_NUMBER";
	case TOK_STRING:
		return "TOK_STRING";
	default:
		return "UNKNOWN";
	}
}
#endif

