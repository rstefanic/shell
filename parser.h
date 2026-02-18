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

typedef struct Expression {
	ExpressionType type;
	union {
		struct {
			AtomKind kind;
			Token value;
		} atom;
		struct ExpressionList {
			Arena *arena;
			struct Expression *expressions;
			u64 size;
			u64 capacity;
		} list;
	} data;
} Expression;

struct ExpressionList parser_new_expression_list(Arena *arena);
void parser_expression_list_push(struct ExpressionList *expressions, Expression expr);

typedef struct Parser Parser;
struct Parser {
	Arena *arena;

	Token *tokens;
	u64 tokens_len;
	u64 tokens_pos;
};

// Primary parsing function
Expression *parse(Arena *arena, Token *tokens, u64 tokens_len);

// Token Parsing Helpers
Token *parser_peek(Parser *parser);
Token *parser_advance(Parser *parser);

// Parser Expression allocators
Expression *new_expression(Parser *parser);

// Expression parsers
Expression *parse_atom(Parser *parser);
Expression *parse_list(Parser *parser);

#endif
