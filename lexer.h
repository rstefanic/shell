#ifndef _LEXERH_
#define _LEXERH_

#include <stdio.h>

#include "base.h"
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
	u64	ptr;	// pointer to current character
};

bool lex(Token* tokens, u64 token_len, String *input);

#endif
