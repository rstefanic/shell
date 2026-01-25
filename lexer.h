#ifndef _LEXERH_
#define _LEXERH_

#include <stdio.h>

#include "memory.h"
#include "string.h"

enum TokenType {
	TOK_EOF,
	TOK_LEFTPAREN,
	TOK_RIGHTPAREN,
	TOK_IDENT,
	TOK_NUMBER,
	TOK_STRING
};
typedef enum TokenType TokenType;

struct Token {
	TokenType	type;
	String		raw;
};
typedef struct Token Token;

typedef struct Lexer Lexer;
struct Lexer {
	String *buf;
	size_t	ptr;	// pointer to current character
};

void lex(Token* tokens, size_t token_len, String *input);

#ifdef DEBUG
const char* debug_token_type_to_string(TokenType type);
#endif

#endif
