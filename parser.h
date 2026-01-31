#ifndef _PARSERH_
#define _PARSERH_

#include <stdbool.h>

#include "lexer.h"

typedef enum ExpressionType {
	EXPR_ATOM,
	EXPR_LIST
} ExpressionType;

typedef enum AtomKind {
	ATOM_IDENT,
	ATOM_NUMBER,
	ATOM_STRING
} AtomKind;

typedef struct Expression Expression;
struct Expression {
	ExpressionType type;
	union {
		struct {
			AtomKind kind;
			Token value;
		} atom;
		struct {
			Expression *children;
			size_t length;
			size_t capacity;
		} list;
	} data;
};

typedef struct Parser Parser;
struct Parser {
	Expression *expressions_buf;
	size_t expressions_len;
	size_t expressions_pos;

	Token *tokens;
	size_t tokens_len;
	size_t tokens_pos;
};

// Primary parsing function
Expression *parse(Expression *expressions, size_t expressions_len, Token *tokens, size_t tokens_len);

// Token Parsing Helpers
Token *peek(Parser *parser);
Token *advance(Parser *parser);

// Parser Expression allocators
Expression *new_expression(Parser *parser);

// Expression parsers
Expression *parse_atom(Parser *parser);
Expression *parse_list(Parser *parser);

#endif
