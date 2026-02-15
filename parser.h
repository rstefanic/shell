#ifndef _PARSERH_
#define _PARSERH_

#include <stdbool.h>

#include "base.h"
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
			Expression *children[10];
			u32 length;
			u32 capacity;
		} list;
	} data;
};

typedef struct Parser Parser;
struct Parser {
	Expression *expressions_buf;
	u32 expressions_len;
	u32 expressions_pos;

	Token *tokens;
	u32 tokens_len;
	u32 tokens_pos;
};

// Primary parsing function
Expression *parse(Expression *expressions, u32 expressions_len, Token *tokens, u32 tokens_len);

// Token Parsing Helpers
Token *parser_peek(Parser *parser);
Token *parser_advance(Parser *parser);

// Parser Expression allocators
Expression *new_expression(Parser *parser);

// Expression parsers
Expression *parse_atom(Parser *parser);
Expression *parse_list(Parser *parser);

#endif
