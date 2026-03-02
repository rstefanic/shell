#include <stdbool.h>
#include <stdio.h>

#include "test.h"
#include "../lexer.h"

// Token buffer for tests
#define MAX_TOKENS 64

// Test results
static u64 tests_run = 0;
static u64 tests_passed = 0;
static u64 tests_failed = 0;

// ---------------------------------------------------------------------------
// Test: empty input
// ---------------------------------------------------------------------------
TEST(test_empty_input) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
}

// ---------------------------------------------------------------------------
// Test: whitespace
// ---------------------------------------------------------------------------
TEST(test_whitespace_spaces) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("foo	bar");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "foo", 3);
	ASSERT_EQ(tokens[1].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[1].raw, "bar", 3);
}

TEST(test_whitespace_tabs) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("foo\t\tbar");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_EQ(tokens[1].type, TOK_IDENT);
}

TEST(test_whitespace_newlines) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("foo\n\nbar");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_EQ(tokens[1].type, TOK_IDENT);
}

TEST(test_leading_trailing_whitespace) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("   foo   ");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "foo", 3);
}

// ---------------------------------------------------------------------------
// Test: parens
// ---------------------------------------------------------------------------
TEST(test_nested_parens) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("(())");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_LEFTPAREN);
	ASSERT_EQ(tokens[1].type, TOK_LEFTPAREN);
	ASSERT_EQ(tokens[2].type, TOK_RIGHTPAREN);
	ASSERT_EQ(tokens[3].type, TOK_RIGHTPAREN);
}

// ---------------------------------------------------------------------------
// Test: numbers
// ---------------------------------------------------------------------------
TEST(test_integer_number) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("123");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_NUMBER);
	ASSERT_STR_EQ(tokens[0].raw, "123", 3);
}

TEST(test_decimal_number) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("3.14159");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_NUMBER);
	ASSERT_STR_EQ(tokens[0].raw, "3.14159", 7);
}

TEST(test_multiple_numbers) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("1 2 3 4 5");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	for (int i = 0; i < 5; i++) {
	  ASSERT_EQ(tokens[i].type, TOK_NUMBER);
	}
}

// ---------------------------------------------------------------------------
// Test: strings
// ---------------------------------------------------------------------------
TEST(test_simple_string) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("\"hello\"");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_STRING);
	ASSERT_STR_EQ(tokens[0].raw, "\"hello\"", 7);
}

TEST(test_string_with_escaped_quote) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("\"hello\\\"world\"");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_STRING);
	// raw includes the escaped quote: hello\"world
	ASSERT_STR_EQ(tokens[0].raw, "\"hello\\\"world\"", 14);
}

TEST(test_empty_string) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("\"\"");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_STRING);
	ASSERT_EQ(tokens[0].raw.len, 2);
}

TEST(test_multiple_strings) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("\"a\" \"b\" \"c\"");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_STRING);
	ASSERT_STR_EQ(tokens[0].raw, "\"a\"", 3);
	ASSERT_EQ(tokens[1].type, TOK_STRING);
	ASSERT_STR_EQ(tokens[1].raw, "\"b\"", 3);
	ASSERT_EQ(tokens[2].type, TOK_STRING);
	ASSERT_STR_EQ(tokens[2].raw, "\"c\"", 3);
}

TEST(test_string_with_spaces) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("\"hello world\"");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_STRING);
	ASSERT_STR_EQ(tokens[0].raw, "\"hello world\"", 13);
}

// ---------------------------------------------------------------------------
// Test: identifiers
// ---------------------------------------------------------------------------
TEST(test_simple_identifier) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("foo");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "foo", 3);
}

TEST(test_ident_quote_prefix) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("'quoted");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "'quoted", 7);
}

TEST(test_ident_dollar_prefix) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("$var");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "$var", 4);
}

TEST(test_ident_bang_prefix) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("!ls");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "!ls", 3);
}

TEST(test_ident_dash_prefix) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("-flag");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "-flag", 5);
}

TEST(test_ident_slash_prefix) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("/path/to/file");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "/path/to/file", 13);
}

TEST(test_ident_with_question) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("empty?");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "empty?", 6);
}

TEST(test_ident_with_numbers) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("var123");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[0].raw, "var123", 6);
}


TEST(test_mixed_expression) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("(foo 123 \"bar\")");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_LEFTPAREN);
	ASSERT_EQ(tokens[1].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[1].raw, "foo", 3);
	ASSERT_EQ(tokens[2].type, TOK_NUMBER);
	ASSERT_STR_EQ(tokens[2].raw, "123", 3);
	ASSERT_EQ(tokens[3].type, TOK_STRING);
	ASSERT_STR_EQ(tokens[3].raw, "\"bar\"", 5);
	ASSERT_EQ(tokens[4].type, TOK_RIGHTPAREN);
}

TEST(test_nested_s_expression) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("(add 1 (mul 2 3))");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_LEFTPAREN);
	ASSERT_EQ(tokens[1].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[1].raw, "add", 3);
	ASSERT_EQ(tokens[2].type, TOK_NUMBER);
	ASSERT_EQ(tokens[3].type, TOK_LEFTPAREN);
	ASSERT_EQ(tokens[4].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[4].raw, "mul", 3);
	ASSERT_EQ(tokens[5].type, TOK_NUMBER);
	ASSERT_EQ(tokens[6].type, TOK_NUMBER);
	ASSERT_EQ(tokens[7].type, TOK_RIGHTPAREN);
	ASSERT_EQ(tokens[8].type, TOK_RIGHTPAREN);
}

TEST(test_complex_nested) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("(defn x (add 1 2))");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(result);
	ASSERT_EQ(tokens[0].type, TOK_LEFTPAREN);
	ASSERT_EQ(tokens[1].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[1].raw, "defn", 4);
	ASSERT_EQ(tokens[2].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[2].raw, "x", 1);
	ASSERT_EQ(tokens[3].type, TOK_LEFTPAREN);
	ASSERT_EQ(tokens[4].type, TOK_IDENT);
	ASSERT_STR_EQ(tokens[4].raw, "add", 3);
	ASSERT_EQ(tokens[5].type, TOK_NUMBER);
	ASSERT_EQ(tokens[6].type, TOK_NUMBER);
	ASSERT_EQ(tokens[7].type, TOK_RIGHTPAREN);
	ASSERT_EQ(tokens[8].type, TOK_RIGHTPAREN);
}

// ---------------------------------------------------------------------------
// Test: unrecognized characters
// ---------------------------------------------------------------------------
TEST(test_unrecognized_char) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("@invalid");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(!result); // assert that lexing failed
}

TEST(test_unrecognized_char_middle) {
	Token tokens[MAX_TOKENS] = {0};
	String input = STR_LIT("(foo @ bar)");

	bool result = lex(tokens, MAX_TOKENS, &input);
	ASSERT_TRUE(!result); // assert that lexing failed
}

int main(void) {
	printf("lexer tests\n\n");

	// whitespace and trimming
	RUN_TEST(test_empty_input);
	RUN_TEST(test_whitespace_spaces);
	RUN_TEST(test_whitespace_tabs);
	RUN_TEST(test_whitespace_newlines);
	RUN_TEST(test_leading_trailing_whitespace);

	// parentheses
	RUN_TEST(test_nested_parens);

	// numbers
	RUN_TEST(test_integer_number);
	RUN_TEST(test_decimal_number);
	RUN_TEST(test_multiple_numbers);

	// strings
	RUN_TEST(test_simple_string);
	RUN_TEST(test_string_with_escaped_quote);
	RUN_TEST(test_empty_string);
	RUN_TEST(test_multiple_strings);
	RUN_TEST(test_string_with_spaces);

	// identifiers
	RUN_TEST(test_simple_identifier);
	RUN_TEST(test_ident_quote_prefix);
	RUN_TEST(test_ident_dollar_prefix);
	RUN_TEST(test_ident_bang_prefix);
	RUN_TEST(test_ident_dash_prefix);
	RUN_TEST(test_ident_slash_prefix);
	RUN_TEST(test_ident_with_question);
	RUN_TEST(test_ident_with_numbers);

	// mixed
	RUN_TEST(test_mixed_expression);
	RUN_TEST(test_nested_s_expression);
	RUN_TEST(test_complex_nested);

	// errors
	RUN_TEST(test_unrecognized_char);
	RUN_TEST(test_unrecognized_char_middle);

	REPORT_TEST_RESULTS();

	return tests_failed > 0 ? 1 : 0;
}
