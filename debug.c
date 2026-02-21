#include "debug.h"

void print_tokens(Token *tokens, u64 token_len) {
	printf("[DEBUG] TOKENS:\n");

	u64 i = 0;
	for (i = 0; i < token_len; i++) {
		Token tok = tokens[i];

		if (tok.type == TOK_EOF)
			break;

		printf("type: %s, raw: [%.*s]\n",
			debug_token_type_to_string(tok.type),
			(int)tok.raw.len,
			tok.raw.value);
	}

	printf("\n\n");
}

void print_expressions(Expression *expressions, u64 indent) {
	if (indent == 0) {
		printf("[DEBUG] EXPRESSIONS:\n");
	}

	Expression *children = expressions->data.list.expressions;
	u64 len = expressions->data.list.size;
	for (u64 i = 0; i < len; i++) {
		Expression child = children[i];
		print_expression(&child, indent+1);
	}
}

void tab(u64 n) {
	for (u64 i = 0; i < n; i++) 
		printf("\t");
}

void print_expression(Expression *expr, u64 indent) {
	tab(indent);
	if (expr->type == EXPR_ATOM) {
		printf("type: EXPR_ATOM, ");
		tab(indent);
		switch (expr->data.atom.kind) {
		case ATOM_IDENT:
			printf("kind: ATOM_IDENT, ");
			break;
		case ATOM_NUMBER:
			printf("kind: ATOM_NUMBER, ");
			break;
		case ATOM_STRING:
			printf("kind: ATOM_STRING, ");
			break;
		}

		Token tok = expr->data.atom.value;
		printf("raw: [%.*s]\n", (int)tok.raw.len, tok.raw.value);
	} else {
		Expression *children = expr->data.list.expressions;
		u64 len = expr->data.list.size;
		for (u64 i = 0; i < len; i++) {
			Expression child = children[i];
			print_expression(&child, indent+1);
		}
	}
}

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
