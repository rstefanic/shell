#include <stdio.h>

#include "../memory.h"
#include "../parser.h"
#include "test.h"

// Test results
static u64 tests_run = 0;
static u64 tests_passed = 0;
static u64 tests_failed = 0;

// Helper function to create tokens
static Token make_token(TokenType type, const char *value, u64 len) {
	return (Token){.type = type, .raw = {.value = (char *)value, .len = len}};
}

// ---------------------------------------------------------------------------
// Test: parse empty program (just EOF)
// ---------------------------------------------------------------------------
TEST(test_parse_empty_program) {
	Arena arena = {0};
	static u8 buffer[KB(16)];
	arena_init(&arena, buffer, KB(16));

	Token tokens[2] = {
		make_token(TOK_EOF, "", 0),
		make_token(TOK_EOF, "", 0)
	};

	Expression *program = parse(&arena, tokens, 2);

	ASSERT_TRUE(program != NULL);
	ASSERT_EQ(program->type, EXPR_LIST);
	ASSERT_EQ(program->data.list.size, 0);

	arena_free(&arena);
}

// ---------------------------------------------------------------------------
// Test: parse atom
// ---------------------------------------------------------------------------
TEST(test_parse_single_atom) {
	Arena arena = {0};
	static u8 buffer[KB(16)];
	arena_init(&arena, buffer, KB(16));

	Token tokens[2] = {
		make_token(TOK_IDENT, "foo", 3),
		make_token(TOK_EOF, "", 0),
	};

	Expression *program = parse(&arena, tokens, 2);

	ASSERT_TRUE(program != NULL);
	ASSERT_EQ(program->type, EXPR_LIST);
	ASSERT_EQ(program->data.list.size, 1);

	Expression *expr = &program->data.list.expressions[0];
	ASSERT_EQ(expr->type, EXPR_ATOM);
	ASSERT_EQ(expr->data.atom.kind, ATOM_IDENT);
	ASSERT_STR_EQ(expr->data.atom.value.raw, "foo", 3);

	arena_free(&arena);
}

// ---------------------------------------------------------------------------
// Test: parse list expression
// ---------------------------------------------------------------------------
TEST(test_parse_simple_list) {
	Arena arena = {0};
	static u8 buffer[KB(16)];
	arena_init(&arena, buffer, KB(16));

	Token tokens[6] = {
		make_token(TOK_LEFTPAREN, "(", 1),
		make_token(TOK_IDENT, "add", 3),
		make_token(TOK_NUMBER, "1", 1),
		make_token(TOK_NUMBER, "2", 1),
		make_token(TOK_RIGHTPAREN, ")", 1),
		make_token(TOK_EOF, "", 0),
	};

	Expression *program = parse(&arena, tokens, 6);

	ASSERT_TRUE(program != NULL);
	ASSERT_EQ(program->type, EXPR_LIST);
	ASSERT_EQ(program->data.list.size, 1);

	Expression *list = &program->data.list.expressions[0];
	ASSERT_EQ(list->type, EXPR_LIST);
	ASSERT_EQ(list->data.list.size, 3);

	// First element: add
	ASSERT_EQ(list->data.list.expressions[0].type, EXPR_ATOM);
	ASSERT_EQ(list->data.list.expressions[0].data.atom.kind, ATOM_IDENT);
	ASSERT_STR_EQ(list->data.list.expressions[0].data.atom.value.raw, "add", 3);

	// Second element: 1
	ASSERT_EQ(list->data.list.expressions[1].type, EXPR_ATOM);
	ASSERT_EQ(list->data.list.expressions[1].data.atom.kind, ATOM_NUMBER);
	ASSERT_STR_EQ(list->data.list.expressions[1].data.atom.value.raw, "1", 1);

	// Third element: 2
	ASSERT_EQ(list->data.list.expressions[2].type, EXPR_ATOM);
	ASSERT_EQ(list->data.list.expressions[2].data.atom.kind, ATOM_NUMBER);
	ASSERT_STR_EQ(list->data.list.expressions[2].data.atom.value.raw, "2", 1);

	arena_free(&arena);
}

// ---------------------------------------------------------------------------
// Test: parse nested list expression
// ---------------------------------------------------------------------------
TEST(test_parse_nested_list) {
	Arena arena = {0};
	static u8 buffer[KB(16)];
	arena_init(&arena, buffer, KB(16));

	Token tokens[10] = {
		make_token(TOK_LEFTPAREN, "(", 1),
		make_token(TOK_IDENT, "add", 3),
		make_token(TOK_NUMBER, "1", 1),
		make_token(TOK_LEFTPAREN, "(", 1),
		make_token(TOK_IDENT, "mul", 3),
		make_token(TOK_NUMBER, "2", 1),
		make_token(TOK_NUMBER, "3", 1),
		make_token(TOK_RIGHTPAREN, ")", 1),
		make_token(TOK_RIGHTPAREN, ")", 1),
		make_token(TOK_EOF, "", 0),
	};

	Expression *program = parse(&arena, tokens, 10);

	ASSERT_TRUE(program != NULL);
	ASSERT_EQ(program->type, EXPR_LIST);
	ASSERT_EQ(program->data.list.size, 1);

	Expression *outer = &program->data.list.expressions[0];
	ASSERT_EQ(outer->type, EXPR_LIST);
	ASSERT_EQ(outer->data.list.size, 3);

	// First element: add
	ASSERT_EQ(outer->data.list.expressions[0].type, EXPR_ATOM);
	ASSERT_EQ(outer->data.list.expressions[0].data.atom.kind, ATOM_IDENT);

	// Second element: 1
	ASSERT_EQ(outer->data.list.expressions[1].type, EXPR_ATOM);
	ASSERT_EQ(outer->data.list.expressions[1].data.atom.kind, ATOM_NUMBER);

	// Third element: nested (mul 2 3)
	Expression *inner = &outer->data.list.expressions[2];
	ASSERT_EQ(inner->type, EXPR_LIST);
	ASSERT_EQ(inner->data.list.size, 3);
	ASSERT_EQ(inner->data.list.expressions[0].data.atom.kind, ATOM_IDENT);
	ASSERT_STR_EQ(inner->data.list.expressions[0].data.atom.value.raw, "mul", 3);

	arena_free(&arena);
}

// ---------------------------------------------------------------------------
// Test: parse builtin command
// ---------------------------------------------------------------------------
TEST(test_parse_builtin) {
	Arena arena = {0};
	static u8 buffer[KB(16)];
	arena_init(&arena, buffer, KB(16));

	// (echo "hello")
	Token tokens[5] = {
		make_token(TOK_LEFTPAREN, "(", 1),
		make_token(TOK_IDENT, "echo", 4),
		make_token(TOK_STRING, "hello", 5),
		make_token(TOK_RIGHTPAREN, ")", 1),
		make_token(TOK_EOF, "", 0),
	};

	Expression *program = parse(&arena, tokens, 5);

	ASSERT_TRUE(program != NULL);
	ASSERT_EQ(program->type, EXPR_LIST);
	ASSERT_EQ(program->data.list.size, 1);

	Expression *expr = &program->data.list.expressions[0];
	ASSERT_EQ(expr->type, EXPR_LIST);
	ASSERT_EQ(expr->data.list.size, 2);

	// First: echo (builtin)
	ASSERT_EQ(expr->data.list.expressions[0].type, EXPR_ATOM);
	ASSERT_EQ(expr->data.list.expressions[0].data.atom.kind, ATOM_IDENT);
	ASSERT_STR_EQ(expr->data.list.expressions[0].data.atom.value.raw, "echo", 4);

	// Second: "hello"
	ASSERT_EQ(expr->data.list.expressions[1].type, EXPR_ATOM);
	ASSERT_EQ(expr->data.list.expressions[1].data.atom.kind, ATOM_STRING);

	arena_free(&arena);
}

// ---------------------------------------------------------------------------
// Test: parse executable
// ---------------------------------------------------------------------------
TEST(test_parse_executable) {
	Arena arena = {0};
	static u8 buffer[KB(16)];
	arena_init(&arena, buffer, KB(16));

	// (!ls "-lah")
	Token tokens[5] = {
		make_token(TOK_LEFTPAREN, "(", 1),
		make_token(TOK_IDENT, "!ls", 3),
		make_token(TOK_STRING, "\"-lah\"", 6),
		make_token(TOK_RIGHTPAREN, ")", 1),
		make_token(TOK_EOF, "", 0),
	};

	Expression *program = parse(&arena, tokens, 5);

	ASSERT_TRUE(program != NULL);
	ASSERT_EQ(program->type, EXPR_LIST);
	ASSERT_EQ(program->data.list.size, 1);

	Expression *expr = &program->data.list.expressions[0];
	ASSERT_EQ(expr->type, EXPR_LIST);
	ASSERT_EQ(expr->data.list.size, 2);

	// First: !ls (executable)
	ASSERT_EQ(expr->data.list.expressions[0].type, EXPR_ATOM);
	ASSERT_EQ(expr->data.list.expressions[0].data.atom.kind, ATOM_IDENT);
	ASSERT_STR_EQ(expr->data.list.expressions[0].data.atom.value.raw, "!ls", 3);

	// Second: "-lah"
	ASSERT_EQ(expr->data.list.expressions[1].type, EXPR_ATOM);
	ASSERT_EQ(expr->data.list.expressions[1].data.atom.kind, ATOM_STRING);
	ASSERT_STR_EQ(expr->data.list.expressions[1].data.atom.value.raw, "\"-lah\"", 6);

	arena_free(&arena);
}

int main(void) {
	printf("parser tests\n\n");

	RUN_TEST(test_parse_empty_program);
	RUN_TEST(test_parse_single_atom);
	RUN_TEST(test_parse_simple_list);
	RUN_TEST(test_parse_nested_list);
	RUN_TEST(test_parse_builtin);
	RUN_TEST(test_parse_executable);

	REPORT_TEST_RESULTS();

	return tests_failed > 0 ? 1 : 0;
}
