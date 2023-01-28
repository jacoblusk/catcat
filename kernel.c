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