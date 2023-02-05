#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "kernel.h"

void function_add_word(struct function f[static 1], struct word *w) {
    if (f->capacity <= (f->size + 1)) {
        f->capacity++;
        f->capacity *= 2;
        f->words = realloc(f->words, sizeof(struct word *) * f->capacity);
    }

    f->words[f->size++] = w;
}

void function_destroy(struct function *function) {
    for (size_t i = 0; i < function->size; i++) {
        word_destroy(function->words[i]);
    }

    free(function->name);
    free(function);
}

void word_value_destroy(struct word *word) {
    switch (word->value.type) {
    case WORD_VALUE_TYPE_INTEGER:
        break;
    case WORD_VALUE_TYPE_STRING:
        free(word->value.string);
        break;
    default:
        fatalf("panic: word_value_destroy called on unsupported type.\n");
    }

    free(word);
}

void word_function_destroy(struct word *word) {
    switch (word->function.type) {
    case FUNCTION_TYPE_CFUNCTION:
        break;
    case FUNCTION_TYPE_REGULAR:
        break;
    case FUNCTION_TYPE_FFI:
        break;
    default:
        fatalf("panic: word_function_destroy called on unsupported type.\n");
    }

    free(word);
}

void word_lambda_destroy(struct word *word) { function_destroy(word->lambda); }

void word_destroy(struct word *word) {
    switch (word->type) {
    case WORD_TYPE_VALUE:
        word_value_destroy(word);
        break;
    case WORD_TYPE_FUNCTION:
        word_function_destroy(word);
        break;
    case WORD_TYPE_LAMBDA:
        word_lambda_destroy(word);
        break;
    default:
        fatalf("panic: word_destroy called on unsupported type.\n");
    }
}

_Bool stack_pop(struct stack *s, struct word **out) {
    if (s->ndata == 0) {
        return 0;
    }

    *out = s->data[--s->ndata];
    return 1;
}

void stack_push(struct stack *s, struct word *in) { s->data[s->ndata++] = in; }

void print_word(struct word *word) {
    switch (word->type) {
    case WORD_TYPE_VALUE:
        switch (word->value.type) {
        case WORD_VALUE_TYPE_INTEGER:
            printf("%lld", word->value.integer);
            break;
        case WORD_VALUE_TYPE_STRING:
            printf("%s", word->value.string);
            break;
        default:
            fatalf("error: unspported value printing\n");
        }
        break;

    case WORD_TYPE_LAMBDA:
        printf("[ ");
        for (size_t i = 0; i < word->lambda->size; i++) {
            print_word(word->lambda->words[i]);
            printf(" ");
        }

        printf("]");
        break;
    case WORD_TYPE_FUNCTION:
        switch (word->function.type) {
        case FUNCTION_TYPE_REGULAR:
            printf("%s", word->function.fn->name);
            break;
        case FUNCTION_TYPE_CFUNCTION:
            printf("%s", word->function.cfn.name);
        }
        break;
    }
}

void stack_print(struct stack *stack) {
    printf("[ ");
    for (size_t i = 0; i < stack->ndata; i++) {
        struct word *w = stack->data[i];
        print_word(w);
        printf(" ");
    }
    printf("]\n");
}

_Bool any_fail(size_t n, ...) {
    va_list vargs;

    va_start(vargs, n);

    for (size_t i = 0; i < n; i++) {
        if (!va_arg(vargs, int)) {
            return 0;
        }
    }

    return 1;
}

void __addfunction(struct environment *env) {
    struct word *a, *b;
    _Bool result;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    if (a->type != WORD_TYPE_VALUE || b->type != WORD_TYPE_VALUE)
        fatalf("error: trying to add non-value types\n");

    if (a->value.type != WORD_VALUE_TYPE_INTEGER ||
        b->value.type != WORD_VALUE_TYPE_INTEGER)
        fatalf("error: trying to add non-capatiable value types\n");

    int64_t add_result         = a->value.integer + b->value.integer;
    struct word *word_result   = calloc(1, sizeof(*word_result));
    word_result->type          = WORD_TYPE_VALUE;
    word_result->value.type    = WORD_VALUE_TYPE_INTEGER;
    word_result->value.integer = add_result;
    stack_push(env->stack, word_result);

    word_destroy(a);
    word_destroy(b);
}

void __equalfunction(struct environment *env) {
    struct word *a, *b;
    _Bool result;
    _Bool are_equal = 1;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    if (a->type != b->type) {
        are_equal = 0;
    } else if (a->type == WORD_TYPE_VALUE) {
        if (a->value.type != b->value.type) {
            are_equal = 0;
        } else {
            switch (a->value.type) {
            case WORD_VALUE_TYPE_INTEGER:
                are_equal = a->value.integer == b->value.integer;
                break;
            case WORD_VALUE_TYPE_STRING:
                are_equal = strcmp(a->value.string, b->value.string) == 0;
                break;
            default:
                fatalf("error: unsupported equality compairson for type.\n");
            }
        }
    } else {
        are_equal = 0;
    }

    int64_t equality_result    = are_equal;
    struct word *word_result   = calloc(1, sizeof(*word_result));
    word_result->type          = WORD_TYPE_VALUE;
    word_result->value.type    = WORD_VALUE_TYPE_INTEGER;
    word_result->value.integer = equality_result;
    stack_push(env->stack, word_result);

    word_destroy(a);
    word_destroy(b);
}

void __mulfunction(struct environment *env) {
    struct word *a, *b;
    _Bool result;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    if (a->type != WORD_TYPE_VALUE || b->type != WORD_TYPE_VALUE)
        fatalf("error: trying to add non-value types\n");

    if (a->value.type != WORD_VALUE_TYPE_INTEGER ||
        b->value.type != WORD_VALUE_TYPE_INTEGER)
        fatalf("error: trying to add non-capatiable value types\n");

    int64_t add_result         = a->value.integer * b->value.integer;
    struct word *word_result   = calloc(1, sizeof(*word_result));
    word_result->type          = WORD_TYPE_VALUE;
    word_result->value.type    = WORD_VALUE_TYPE_INTEGER;
    word_result->value.integer = add_result;
    stack_push(env->stack, word_result);

    word_destroy(a);
    word_destroy(b);
}

void __applyfunction(struct environment *env) {
    struct word *a;
    _Bool result;

    result = stack_pop(env->stack, &a);
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    if (a->type != WORD_TYPE_LAMBDA)
        fatalf("error: trying to apply to a non-lambda type\n");

    struct environment subenv;
    environment_copy(&subenv, env);
    subenv.entry = a->lambda;
    environment_execute(&subenv);

    word_destroy(a);
}

void __printfunction(struct environment *env) {
    struct word *a;
    _Bool result;

    result = stack_pop(env->stack, &a);
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    print_word(a);
    printf("\n");

    word_destroy(a);
}

void __printsfunction(struct environment *env) { stack_print(env->stack); }

void __dropfunction(struct environment *env) {
    struct word *a;
    _Bool result;

    result = stack_pop(env->stack, &a);
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    word_destroy(a);
}

void __swapfunction(struct environment *env) {
    struct word *a, *b;
    _Bool result;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    stack_push(env->stack, b);
    stack_push(env->stack, a);
}

void __rotfunction(struct environment *env) {
    struct word *a, *b, *c;
    _Bool result;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b),
                      stack_pop(env->stack, &c));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    stack_push(env->stack, b);
    stack_push(env->stack, c);
    stack_push(env->stack, a);
}

void lambda_copy(struct function *dest, struct function *src) {
    memcpy(dest, src, sizeof(*dest));

    dest->words = malloc(sizeof(struct word *) * sizeof(dest->size));
    for (size_t i = 0; i < dest->size; i++) {
        dest->words[i] = malloc(sizeof(struct word));
        memcpy(dest->words[i], src->words[i], sizeof(struct word));
    }
}

void word_copy(struct word *dest, struct word *src) {
    memcpy(dest, src, sizeof(*dest));

    switch (src->type) {
    case WORD_TYPE_LAMBDA:
        dest->lambda = malloc(sizeof(struct function));
        lambda_copy(dest->lambda, src->lambda);
        break;
    default:
        break;
    }
}

void __bifunction(struct environment *env) {
    struct word *a, *b, *c;
    _Bool result;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b),
                      stack_pop(env->stack, &c));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    if (b->type != WORD_TYPE_LAMBDA || c->type != WORD_TYPE_LAMBDA)
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

void __composefunction(struct environment *env) {
    struct word *a, *b, *c;
    _Bool result;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    if (a->type != WORD_TYPE_LAMBDA && b->type != WORD_TYPE_LAMBDA)
        fatalf("error: compose operating on non lambda type.\n");

    struct function *afn = a->lambda;
    struct function *bfn = b->lambda;

    c         = calloc(1, sizeof(*c));
    c->type   = WORD_TYPE_LAMBDA;
    c->lambda = afn;

    for (size_t i = 0; i < bfn->size; i++) {
        function_add_word(c->lambda, bfn->words[i]);
    }

    stack_push(env->stack, c);

    word_destroy(a);
    word_destroy(b);
}

void __curryfunction(struct environment *env) {
    struct word *a, *b, *c;
    _Bool result;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    if (b->type != WORD_TYPE_LAMBDA)
        fatalf("error: curry is operating on non lambda type.\n");

    struct function *bfn = b->lambda;
    c                    = calloc(1, sizeof(*c));
    c->type              = WORD_TYPE_LAMBDA;
    c->lambda            = bfn;

    function_add_word(c->lambda, a);
    struct word *temp   = c->lambda->words[0];
    c->lambda->words[0] = c->lambda->words[c->lambda->size - 1];
    c->lambda->words[c->lambda->size - 1] = temp;

    stack_push(env->stack, c);

    word_destroy(a);
    word_destroy(b);
}

void __timesfunction(struct environment *env) {
    struct word *a, *b;
    _Bool result;

    result = any_fail(2, stack_pop(env->stack, &a), stack_pop(env->stack, &b));
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    if (a->type != WORD_TYPE_VALUE ||
        (a->type == WORD_TYPE_VALUE &&
         a->value.type != WORD_VALUE_TYPE_INTEGER) ||
        b->type != WORD_TYPE_LAMBDA)
        fatalf("error: times expects an integer and a lambda\n");

    for (int64_t i = 0; i < a->value.integer; i++) {
        struct word *c = calloc(1, sizeof(*c));
        word_copy(c, b);

        stack_push(env->stack, c);
        __applyfunction(env);
    }

    word_destroy(a);
    word_destroy(b);
}

void __dupfunction(struct environment *env) {
    struct word *a, *b;
    _Bool result;

    b      = malloc(sizeof(*a));
    result = stack_pop(env->stack, &a);
    if (!result)
        fatalf("error: stack_pop failed, empty stack\n");

    b = malloc(sizeof(*a));
    word_copy(b, a);

    stack_push(env->stack, a);
    stack_push(env->stack, b);
}

struct environment *make_environment() {
    struct environment *env = calloc(1, sizeof(*env));
    env->stack              = calloc(1, sizeof(*env->stack));
    return env;
}

void environment_copy(struct environment dest[static 1],
                      struct environment src[static 1]) {
    memcpy(dest, src, sizeof(struct environment));
}

void environment_execute(struct environment *env) {
    for (int i = 0; i < env->entry->size; i++) {
        struct word *w = env->entry->words[i];

        switch (w->type) {
        case WORD_TYPE_LAMBDA:
        case WORD_TYPE_VALUE: {
            struct word *w_copy = calloc(1, sizeof(*w_copy));
            word_copy(w_copy, w);

            stack_push(env->stack, w_copy);
            break;
        }
        case WORD_TYPE_FUNCTION:
            if (w->function.type == FUNCTION_TYPE_CFUNCTION) {
                w->function.cfn.function(env);
            } else if (w->function.type == FUNCTION_TYPE_REGULAR) {
                struct environment subenv;
                environment_copy(&subenv, env);
                subenv.entry = w->function.fn;
                environment_execute(&subenv);
            } else if (w->function.type == FUNCTION_TYPE_FFI) {
#ifdef ENABLE_FFI
                struct ffi_function *fn = w->function.ffi_fn;
                void *values[fn->nargs];

                for (int i = 0; i < fn->nargs; i++) {
                    struct word *a;

                    if (!stack_pop(env->stack, &a))
                        fatalf("error: stack_pop failed, empty stack\n");

                    if (a->type != WORD_TYPE_VALUE)
                        fatalf("error: ffi functions only accept values.\n");

                    switch (a->value.type) {
                    case WORD_VALUE_TYPE_INTEGER:
                        values[i] = &(a->value.integer);
                        break;
                    case WORD_VALUE_TYPE_STRING:
                        values[i] = &(a->value.string);
                        break;
                    default:
                        fatalf("error: unsupported word value type for ffi\n");
                    }

                    // free(a);
                }

                ffi_arg result;
                ffi_call(&fn->cif, fn->fn, &result, values);

                struct word *word_result   = calloc(1, sizeof(*word_result));
                word_result->type          = WORD_TYPE_VALUE;
                word_result->value.type    = WORD_VALUE_TYPE_INTEGER;
                word_result->value.integer = result;
                stack_push(env->stack, word_result);
#else
                fatalf("FFI support not enabled.");
#endif
            }
            break;
        }
    }
}