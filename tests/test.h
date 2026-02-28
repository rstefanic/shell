/**
* This file contains macros to create a test suite. The test suite which uses
* these macros must include the following at the start of the test file:
*
*	static u64 tests_run = 0;
*	static u64 tests_passed = 0;
*	static u64 tests_failed = 0;
*
* These macros operate on these values to track the testing.
*/
#include <stdlib.h>
#include <string.h>

#include "../base.h"
#include "../log.h"
#include "../string.h"

// Creates a test with this name.
#define TEST(name) static void name(void)

// Runs a test by its name. Since some tests require a log context, so a dummy
// log context here is created but its contents are never read.
// TODO: Refactor the to skip if no log context has been established?
#define RUN_TEST(name)				\
	do {					\
		printf("  %s... ", #name);	\
		log_context_start();		\
		name();				\
		log_context_end();		\
		tests_run++;			\
		printf("OK\n");			\
	} while (0)

#define ASSERT_TRUE(expr)								\
	do {										\
		if (!(expr)) {								\
			printf(								\
				"FAIL\n	assertion failed: %s\n	  at %s:%d\n", #expr,	\
				__FILE__, __LINE__					\
			);								\
			tests_failed++;							\
			return;								\
		}									\
		tests_passed++;								\
	} while (0)

#define ASSERT_EQ(a, b)									\
	do {										\
		if ((a) != (b)) {							\
			printf(								\
				"FAIL\n    expected %lld == %lld\n    %s:%d\n",		\
				(long long)(a), (long long)(b), __FILE__, __LINE__	\
			);								\
			tests_failed++;							\
			return;								\
		}									\
		tests_passed++;								\
	} while (0)

#define ASSERT_STR_EQ(str, expected, expected_len)						\
	do {											\
		if (strncmp((str).value, (expected), (expected_len)) != 0 ||			\
			(str).len != (expected_len))						\
		{										\
			printf(									\
				"FAIL\n    expected \"%.*s\" == \"%s\"\n	  %s:%d\n",	\
				(int)(str).len, (str).value, (expected), __FILE__, __LINE__	\
			);									\
			tests_failed++;								\
			return;									\
		}										\
		tests_passed++;									\
	} while (0)

#define REPORT_TEST_RESULTS()						\
	do {								\
		printf("\n----------------------------------------\n"); \
		printf("tests run: %lu\n", tests_run);			\
		printf("passed:    %lu\n", tests_passed);		\
		printf("failed:    %lu\n", tests_failed);		\
		printf("----------------------------------------\n");	\
	} while (0)

