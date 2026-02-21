#ifndef _DEBUGH_
#define _DEBUGH_

#include "base.h"
#include "lexer.h"
#include "parser.h"

const char* debug_token_type_to_string(TokenType type);
void print_tokens(Token *tokens, u64 token_len);
void print_expression(Expression *expr, u64 indent);
void print_expressions(Expression *expressions, u64 indent);
void tab(u64 n);
void print_expression(Expression *expr, u64 indent);

#endif
