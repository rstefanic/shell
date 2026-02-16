#include <assert.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "log.h"

Expression *parse(Expression *expressions, u64 expressions_len, Token *tokens, u64 tokens_len) {
	Parser parser = (Parser) {
		.expressions_buf = expressions,
		.expressions_len = expressions_len,
		.expressions_pos = 0,
		.tokens = tokens,
		.tokens_len = tokens_len,
		.tokens_pos = 0
	};

	// This is the root node of the AST.
	Expression *program = new_expression(&parser);
	assert(program != NULL);
	program->type = EXPR_LIST;
	program->data.list.length = 0;
	program->data.list.capacity = 10;

	// Keep parsing the program expressions until we hit EOF.
	// NOTE: This is similar to the `parse_list` function since a program
	// is also just a list of expressions.
	while (parser_peek(&parser)->type != TOK_EOF) {
		u64 len = program->data.list.length;
		assert(len < program->data.list.capacity);

		Token *token = parser_peek(&parser);
		if (token->type == TOK_LEFTPAREN) {
			program->data.list.children[len] = parse_list(&parser);
		} else {
			program->data.list.children[len] = parse_atom(&parser);
		}

		program->data.list.length += 1;
	}

	char buf[256] = { 0 };
	sprintf(
		buf,
		"(parser) parse memory used: %lu bytes\n",
		sizeof(Expression) * parser.expressions_pos
	);
	log_emit_message(LOG_LEVEL_INFO, (String) {
		.value = buf,
		.len = strlen(buf)
	});

	return program;
}

Token *parser_peek(Parser *parser) {
	return &parser->tokens[parser->tokens_pos];
}

Token *parser_advance(Parser *parser) {
	if (parser->tokens_pos >= parser->tokens_len) {
		return NULL;
	}

	Token *token = parser_peek(parser);
	parser->tokens_pos += 1;
	return token;
}

Expression *new_expression(Parser *parser) {
	// Out of Memory check
	if (parser->expressions_pos >= parser->expressions_len) {
		return NULL;
	}

	Expression *expr = &parser->expressions_buf[parser->expressions_pos];
	parser->expressions_pos += 1;
	return expr;
}

Expression *parse_atom(Parser *parser) {
	Token *token = parser_advance(parser); // consume the atom

	Expression *expression = new_expression(parser);
	assert(expression != NULL);
	expression->type = EXPR_ATOM;
	expression->data.atom.value = *token;

	switch (token->type) {
	case TOK_IDENT:
		expression->data.atom.kind = ATOM_IDENT;
		break;
	case TOK_NUMBER:
		expression->data.atom.kind = ATOM_NUMBER;
		break;
	case TOK_STRING:
		expression->data.atom.kind = ATOM_STRING;
		break;
	default:
		break;
	};

	return expression;
}

Expression *parse_list(Parser *parser) {
	// Consume the TOK_LEFTPAREN and peek the next token.
	parser_advance(parser);

	// Start a new list expression
	Expression *expr = new_expression(parser);
	assert(expr != NULL);
	expr->type = EXPR_LIST;
	expr->data.list.length = 0;
	expr->data.list.capacity = 10; // matches the size of `**children`

	// TODO: Don't exceed grabbing tokens outside of bounds of token length.
	while (true) {
		// Ensure we have enough list capacity to parse this.
		u64 len = expr->data.list.length;
		assert(expr->data.list.capacity > len);

		Token *token = parser_peek(parser);
		switch (token->type) {
		case TOK_EOF:
			return expr;
		case TOK_LEFTPAREN:
			expr->data.list.children[len] = parse_list(parser);
			break;
		case TOK_RIGHTPAREN:
			parser_advance(parser); // consume the ')'
			return expr;
		default:
			expr->data.list.children[len] = parse_atom(parser);
		}

		expr->data.list.length += 1;
	}
}

