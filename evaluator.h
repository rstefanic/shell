#ifndef _EVALUATOR_H_
#define _EVALUATOR_H_

#include <assert.h>
#include <string.h>

#include "memory.h"
#include "hashtable.h"
#include "parser.h"

typedef enum {
	BC_NONE,
	BC_CD,
	BC_PWD,
	BC_EXIT,
	BC_ECHO
} BuiltinCommand;

typedef enum {
	VAL_NIL,
	VAL_NUMBER,
	VAL_STRING,
	VAL_SYMBOL,
	VAL_BUILTIN,
	VAL_EXECUTABLE
} ValueType;

typedef struct Value {
	ValueType type;
	union {
		i64	number;
		String	*string;
		String	*symbol;
		BuiltinCommand cmd;
	} data;
} Value;

typedef struct StackFrame {
	struct StackFrame *prev_frame;	// NULL indicates first stack frame
	Expression *expression;		// expression this frame is evaluating
	Value *values;			// evaluated expression results
	u64 values_len;
} StackFrame;

typedef struct {
	Arena *arena;
	HashTable *symtable;
	StackFrame *stack_frame;
} Runtime;

Runtime create_runtime(Arena *arena);
Value *evaluator_eval_expressions(Runtime *runtime, Expression *expr);

Value handle_builtin(Runtime *runtime, StackFrame *stack_frame);
void interpolate_string(Runtime *runtime, char* src, u64 srclen, char* dest, u64 destlen);
Value execute_program(Runtime *runtime);

#endif
