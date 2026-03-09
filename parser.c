#include <assert.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "log.h"
#include "memory.h"

Expression *parse(Arena *arena, Token *tokens, u64 tokens_len) {
	// Take the start offset so that we can calculate how much memory this
	// function has consumed by the end and add it to the Info Report.
	u64 start_offset = arena->curr_offset;

	Parser parser = (Parser) {
		.arena = arena,
		.tokens = tokens,
		.tokens_len = tokens_len,
		.tokens_pos = 0
	};

	// This is the root node of the AST.
	Expression *program = new_expression(&parser);
	program->type = EXPR_LIST;
	program->data.list = parser_new_expression_list(arena);

	// Keep parsing the program expressions until we hit EOF.
	// NOTE: This is similar to the `parse_list` function since a program
	// is also just a list of expressions.
	while (parser_peek(&parser)->type != TOK_EOF) {
		Token *token = parser_peek(&parser);
		if (token->type == TOK_LEFTPAREN) {
			parser_expression_list_push(&program->data.list, *parse_list(&parser));
		} else {
			parser_expression_list_push(&program->data.list, *parse_atom(&parser));
		}
	}

	char buf[256] = { 0 };
	sprintf(
		buf,
		"(parser) parse memory used: %llu bytes\n",
		arena->curr_offset - start_offset
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
	Expression *new = arena_alloc(parser->arena, sizeof(Expression));
	assert(new != NULL);
	return new;
}

Expression *parse_atom(Parser *parser) {
	Token *token = parser_advance(parser); // consume the atom

	Expression *expression = new_expression(parser);
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
	expr->type = EXPR_LIST;
	expr->data.list = parser_new_expression_list(parser->arena);

	while (true) {
		// TODO: Don't exceed grabbing tokens outside of bounds of token length.
		Token *token = parser_peek(parser);
		switch (token->type) {
		case TOK_EOF:
			return expr;
		case TOK_LEFTPAREN:
			parser_expression_list_push(&expr->data.list, *parse_list(parser));
			break;
		case TOK_RIGHTPAREN:
			parser_advance(parser); // consume the ')'
			return expr;
		default:
			parser_expression_list_push(&expr->data.list, *parse_atom(parser));
		}
	}
}


struct ExpressionList parser_new_expression_list(Arena *arena) {
	u64 capacity = 8;
	Expression *expressions = arena_alloc(arena, sizeof(Expression) * capacity);
	assert(expressions != NULL);
	return (struct ExpressionList){
		.arena = arena,
		.expressions = expressions,
		.size = 0,
		.capacity = capacity
	};
}

void parser_expression_list_push(struct ExpressionList *expressions, Expression expr) {
	// If we've run out of space, double the capacity.
	if (expressions->size >= expressions->capacity) {
		Expression *expanded = arena_alloc(
			expressions->arena,
			sizeof(Expression) * expressions->capacity * 2
		);
		assert(expanded != NULL);

		// NOTE: This is a bit wasteful as we're just losing a reference
		// to the old memory. The memory will be cleaned up but this is
		// technically a leak that will need to be patched later. This
		// shouldn't be too bad though since it's reasonable to assume
		// that most lists right now will have less than 8 expressions.
		memcpy(expanded, expressions->expressions, sizeof(Expression) * expressions->capacity);
		expressions->expressions = expanded;
		expressions->capacity *= 2;
	}

	expressions->expressions[expressions->size++] = expr;
}
