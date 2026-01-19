#ifndef _LEXERH_
#define _LEXERH_

#include <stdio.h>

#include "memory.h"

enum TokenType {
	EMPTY,
	LITERAL
};
typedef enum TokenType TokenType;

struct Token {
	TokenType	type;
	char		*raw;	// pointer to the token's characters
	size_t		len;	// size of the token's underlying string
};
typedef struct Token Token;

typedef struct Lexer Lexer;
struct Lexer {
	char	*buf;	// underlying buffer we're lexing
	size_t	len;	// length of the underlying buffer
	size_t	ptr;	// pointer to current character
};

void lex(Token* tokens, size_t token_len, char *input, size_t input_len);

#endif
