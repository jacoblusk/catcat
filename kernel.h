#ifndef KERNEL_H
#define KERNEL_H

#define WIN32_LEAN_AND_MEAN

#ifdef ENABLE_FFI
#include <ffi.h>
#include <windows.h>
#endif

#include "parser.h"

struct word;

struct function {
    char *name;
    struct word **words;
    size_t size;
    size_t capacity;
};

enum word_type { WORD_TYPE_LAMBDA, WORD_TYPE_VALUE, WORD_TYPE_FUNCTION };

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

enum function_type {
    FUNCTION_TYPE_CFUNCTION,
    FUNCTION_TYPE_FFI,
    FUNCTION_TYPE_REGULAR
};

#ifdef ENABLE_FFI
struct ffi_function {
    char *name;
    ffi_cif cif;
    ffi_type *args[8];
    ffi_type ret_type;
    size_t nargs;
    FARPROC fn;
};
#endif

struct word_function {
    enum function_type type;
    union {
        struct internal_function cfn;
#ifdef ENABLE_FFI
        struct ffi_function *ffi_fn;
#endif
        struct function *fn;
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
    struct word **globals;
    size_t globals_capacity;
    size_t globals_size;
    struct function *entry;
    struct stack *stack;
};

_Bool stack_pop(struct stack *stack, struct word **out);
void stack_push(struct stack *stack, struct word *in);

void __addfunction(struct environment *env);
void __mulfunction(struct environment *env);
void __applyfunction(struct environment *env);
void __printfunction(struct environment *env);
void __dropfunction(struct environment *env);
void __dupfunction(struct environment *env);
void __swapfunction(struct environment *env);
void __bifunction(struct environment *env);
void __timesfunction(struct environment *env);
void __rotfunction(struct environment *env);
void __composefunction(struct environment *env);
void __curryfunction(struct environment *env);
void __printsfunction(struct environment *env);
void __equalfunction(struct environment *env);
void __putstestffifunction(struct environment *env);

void environment_copy(struct environment *dest, struct environment *src);
void environment_execute(struct environment *env);
struct environment *make_environment();

void word_copy(struct word *dest, struct word *src);
void word_value_destroy(struct word *word);
void word_function_destroy(struct word *word);
void word_lambda_destroy(struct word *word);
void word_destroy(struct word *word);

void function_destroy(struct function *function);
void function_add_word(struct function *function, struct word *word);

#endif