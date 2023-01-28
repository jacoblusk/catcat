#ifndef KERNEL_H
#define KERNEL_H

#include "parser.h"

struct word;

struct function {
	char *name;
	struct word **words;
	size_t size;
	size_t capacity;
};

enum word_type {
	WORD_TYPE_LAMBDA,
	WORD_TYPE_VALUE,
	WORD_TYPE_FUNCTION
};

enum word_value_type {
	WORD_VALUE_TYPE_STRING,
	WORD_VALUE_TYPE_INTEGER,
	WORD_VALUE_TYPE_ANY
};

struct word_value {
	enum word_value_type type;
	union {
		char *string;
		int64_t integer;
		void *any;
	};
};


struct environment;

typedef void (*cfunction)(struct environment *env);

struct internal_function {
	char const *name;
	cfunction function;
};

struct word_function {
	_Bool is_cfunction;
	union {
		cfunction cfunction;
		struct function *function;
	};
};

struct word {
	enum word_type type;
	union {
		struct function *lambda;
		struct word_value value;
		struct word_function function;
	};
};

#define NDATA 256
struct stack {
	struct word *data[NDATA];
	size_t ndata;
};

struct environment {
	struct function **globals;
	size_t globals_capacity;
	size_t globals_size;
	struct function *entry;
	struct stack *stack;
};

_Bool stack_pop(struct stack s[static 1], struct word **out);
void stack_push(struct stack s[static 1], struct word *in);

void __addfunction(struct environment env[static 1]);
void __mulfunction(struct environment env[static 1]);
void __applyfunction(struct environment env[static 1]);
void __printfunction(struct environment env[static 1]);
void __dropfunction(struct environment env[static 1]);
void __dupfunction(struct environment env[static 1]);
void __swapfunction(struct environment env[static 1]);
void __bifunction(struct environment env[static 1]);
void __rotfunction(struct environment env[static 1]);

void environment_copy(struct environment *dest, struct environment *src);
void environment_execute(struct environment *env);
struct environment *make_environment();

#endif