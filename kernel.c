#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "kernel.h"
#include "error.h"

_Bool stack_pop(struct stack s[static 1], struct word *out[static 1]) {
	if(s->ndata == 0) {
		return 0;
	}

	*out = s->data[--s->ndata];
	return 1;
}

void stack_push(struct stack s[static 1], struct word in[static 1]) {
	s->data[s->ndata++] = in;
}

_Bool any_fail(size_t n, ...) {
	va_list vargs;

	va_start(vargs, n);

	for(size_t i = 0; i < n; i++) {
		if(!va_arg(vargs, int)) {
			return 0;
		}
	}

	return 1;
}

void __addfunction(struct environment env[static 1]) {
	struct word *a, *b;
	_Bool result;

	result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	if(a->type != WORD_TYPE_VALUE || b->type != WORD_TYPE_VALUE)
		fatalf("error: trying to add non-value types\n");

	if(a->value.type != WORD_VALUE_TYPE_INTEGER || b->value.type != WORD_VALUE_TYPE_INTEGER)
		fatalf("error: trying to add non-capatiable value types\n");

	int64_t add_result = a->value.integer + b->value.integer;
	struct word *word_result = calloc(1, sizeof(*word_result));
	word_result->type = WORD_TYPE_VALUE;
	word_result->value.type = WORD_VALUE_TYPE_INTEGER;
	word_result->value.integer = add_result;
	stack_push(env->stack, word_result);

	free(a);
	free(b);
}

void __mulfunction(struct environment env[static 1]) {
	struct word *a, *b;
	_Bool result;

	result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	if(a->type != WORD_TYPE_VALUE || b->type != WORD_TYPE_VALUE)
		fatalf("error: trying to add non-value types\n");

	if(a->value.type != WORD_VALUE_TYPE_INTEGER || b->value.type != WORD_VALUE_TYPE_INTEGER)
		fatalf("error: trying to add non-capatiable value types\n");

	int64_t add_result = a->value.integer * b->value.integer;
	struct word *word_result = calloc(1, sizeof(*word_result));
	word_result->type = WORD_TYPE_VALUE;
	word_result->value.type = WORD_VALUE_TYPE_INTEGER;
	word_result->value.integer = add_result;
	stack_push(env->stack, word_result);

	free(a);
	free(b);
}

void __applyfunction(struct environment env[static 1]) {
	struct word *a;
	_Bool result;

	result = stack_pop(env->stack, &a);
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	if(a->type != WORD_TYPE_LAMBDA)
		fatalf("error: trying to apply to a non-lambda type\n");

	struct environment subenv;
	environment_copy(&subenv, env);
	subenv.entry = a->lambda;
	environment_execute(&subenv);

	free(a);
}

void __printvalue(struct word a[static 1]) {
	switch(a->value.type) {
	case WORD_VALUE_TYPE_INTEGER:
		printf("%lld\n", a->value.integer);
		break;
	default:
		fatalf("error: trying to print unsupported value");
	}
}

void __printfunction(struct environment env[static 1]) {
	struct word *a;
	_Bool result;

	result = stack_pop(env->stack, &a);
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	switch(a->type) {
		case WORD_TYPE_VALUE: {
			__printvalue(a);
			break;
		}
		case WORD_TYPE_LAMBDA: {
			printf("lambda:%p\n", &a->lambda);
			break;
		}
		case WORD_TYPE_FUNCTION: {
			printf("fn:%p\n", &a->function.cfunction);
			break;
		}
	}

	free(a);
}


void __dropfunction(struct environment env[static 1]) {
	struct word *a;
	_Bool result;

	result = stack_pop(env->stack, &a);
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	free(a);
}

void __swapfunction(struct environment env[static 1]) {
	struct word *a, *b;
	_Bool result;

	result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	stack_push(env->stack, b);
	stack_push(env->stack, a);
}

void __rotfunction(struct environment env[static 1]) {
	struct word *a, *b, *c;
	_Bool result;

	result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b), stack_pop(env->stack, &c));
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	stack_push(env->stack, b);
	stack_push(env->stack, c);
	stack_push(env->stack, a);
}


void lambda_copy(struct function *dest, struct function *src) {
	memcpy(dest, src, sizeof(*dest));

	dest->words = malloc(sizeof(struct word *) * sizeof(dest->size));
	for(size_t i = 0; i < dest->size; i++) {
		dest->words[i] = malloc(sizeof(struct word));
		memcpy(dest->words[i], src->words[i], sizeof(struct word));
	}
}

void word_copy(struct word *dest, struct word *src) {
	memcpy(dest, src, sizeof(*dest));

	switch(src->type) {
		case WORD_TYPE_LAMBDA:
			dest->lambda = malloc(sizeof(struct function));
			lambda_copy(dest->lambda, src->lambda);
			break;
		default:
			break;
	}
}

void __bifunction(struct environment env[static 1]) {
	struct word *a, *b, *c;
	_Bool result;

	result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b), stack_pop(env->stack, &c));
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	if(b->type != WORD_TYPE_LAMBDA || c->type != WORD_TYPE_LAMBDA)
		fatalf("error: bi operating on non lambda type.\n");

	struct word *d = calloc(1, sizeof(*d));
	word_copy(d, a);

	stack_push(env->stack, a);
	stack_push(env->stack, b);
	__applyfunction(env);

	stack_push(env->stack, d);
	stack_push(env->stack, c);
	__applyfunction(env);
}

void __timesfunction(struct environment env[static 1]) {
	struct word *a, *b;
	_Bool result;

	result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	if(a->type != WORD_TYPE_VALUE || (a->type == WORD_TYPE_VALUE && a->value.type != WORD_VALUE_TYPE_INTEGER) || \
	   b->type != WORD_TYPE_LAMBDA)
		fatalf("error: times expects an integer and a lambda\n");

	for(int64_t i = 0; i < a->value.integer; i++) {
		struct word *c = calloc(1, sizeof(*c));
		word_copy(c, b);

		stack_push(env->stack, c);
		__applyfunction(env);
	}

	free(a); free(b);
}

void __dupfunction(struct environment env[static 1]) {
	struct word *a, *b;
	_Bool result;

	b = malloc(sizeof(*a));
	result = stack_pop(env->stack, &a);
	if(!result)
		fatalf("error: stack_pop failed, empty stack\n");

	memcpy(b, a, sizeof(*a));

	stack_push(env->stack, a);
	stack_push(env->stack, b);
}

struct environment *make_environment() {
	struct environment *env = calloc(1, sizeof(*env));
	env->stack = calloc(1, sizeof(*env->stack));
	return env;
}

void environment_copy(struct environment dest[static 1], struct environment src[static 1]) {
	memcpy(dest, src, sizeof(struct environment));
}

void environment_execute(struct environment env[static 1]) {
	for(int i = 0; i < env->entry->size; i++) {
		struct word *w = env->entry->words[i];

		switch(w->type) {
		case WORD_TYPE_LAMBDA:
		case WORD_TYPE_VALUE:
			stack_push(env->stack, w);
			break;
		case WORD_TYPE_FUNCTION:
			if(w->function.is_cfunction) {
				w->function.cfunction(env);
			} else {
				struct environment subenv;
				environment_copy(&subenv, env);
				subenv.entry = w->function.function;
				environment_execute(&subenv);
			}
			break;
		}
	}
}