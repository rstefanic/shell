#ifndef _LEXERH_
#define _LEXERH_

#include <stdio.h>

#include "memory.h"
#include "string.h"

enum TokenType {
	EMPTY,
	LITERAL
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

#endif
