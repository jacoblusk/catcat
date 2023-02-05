#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_FFI
#include <ffi.h>
#include <windows.h>
#endif

#include "error.h"
#include "parser.h"

#define GET_NEXT_TOKEN(p, t)                                                   \
    do {                                                                       \
        token_destroy((t));                                                    \
        tokens_pop(&(p)->tokens, &(t));                                        \
    } while (0)

#ifndef ENABLE_FFI
char *strdup(char const *str) {
    char *copy = malloc(strlen(str) + 1);
    memcpy(copy, str, strlen(str) + 1);
    return copy;
}
#endif

void parser_errorf(struct parser *parser, char *fmt, ...) {
    va_list vargs, vargsd;

    va_start(vargs, fmt);
    va_copy(vargsd, vargs);

    int msg_size = vsnprintf(NULL, 0, fmt, vargs);
    if (msg_size < 0)
        fatalf("error: vsnprintf %d\n", msg_size);

    char *error_msg = malloc(msg_size + 1);
    vsnprintf(error_msg, msg_size, fmt, vargs);

    parser->error.message  = error_msg;
    parser->error.needfree = 1;
}

void parser_set_error(struct parser *parser, char *message, _Bool needfree) {
    parser->error.message  = message;
    parser->error.needfree = needfree;
}

_Bool parser_success(struct parser *parser) {
    return parser->error.message == NULL;
}

void parser_error_finish(struct parser *parser) {
    if (parser->error.needfree)
        free(parser->error.message);
    parser->error.message = NULL;
}

static struct internal_function internal_functions[] = {
    {"apply", __applyfunction},
    {"print", __printfunction},
    {"prints", __printsfunction},
    {"dup", __dupfunction},
    {"swap", __swapfunction},
    {"rot", __rotfunction},
    {"bi", __bifunction},
    {"times", __timesfunction},
    {"+", __addfunction},
    {"*", __mulfunction},
    {".", __dropfunction},
    {"compose", __composefunction},
    {"curry", __curryfunction},
    {"equal?", __equalfunction},
    {NULL, NULL}};

struct function *make_function(char const *name) {
    struct function *f = malloc(sizeof(*f));
    f->name            = strdup("[lambda]");

    if (name)
        f->name = strdup(name);

    f->words    = calloc(NFUNCTION_WORDS, sizeof(struct word));
    f->capacity = NFUNCTION_WORDS;
    f->size     = 0;

    return f;
}

struct word *make_word_string(char const *string) {
    struct word *word = NULL;

    word               = malloc(sizeof(*word));
    word->type         = WORD_TYPE_VALUE;
    word->value.type   = WORD_VALUE_TYPE_STRING;
    word->value.string = strdup(string);

    return word;
}

struct word *make_word_integer(uint64_t value) {
    struct word *word = NULL;

    word                = malloc(sizeof(*word));
    word->type          = WORD_TYPE_VALUE;
    word->value.type    = WORD_VALUE_TYPE_INTEGER;
    word->value.integer = value;

    return word;
}

struct word *make_word_cfunction(const char *name, cfunction cfn) {
    struct word *word = NULL;

    word                        = calloc(1, sizeof(*word));
    word->type                  = WORD_TYPE_FUNCTION;
    word->function.type         = FUNCTION_TYPE_CFUNCTION;
    word->function.cfn.function = cfn;
    word->function.cfn.name     = strdup(name);

    return word;
}

struct word *make_word_regular_function(struct function *fn) {
    struct word *word = NULL;

    word                = calloc(1, sizeof(*word));
    word->type          = WORD_TYPE_FUNCTION;
    word->function.type = FUNCTION_TYPE_REGULAR;
    word->function.fn   = fn;

    return word;
}

void environment_add_global(struct environment *env, struct word *function) {
    if (env->globals_capacity <= (env->globals_size + 1)) {
        env->globals_capacity++;
        env->globals_capacity *= 2;
        env->globals = realloc(env->globals, sizeof(struct function *) *
                                                 env->globals_capacity);
    }

    env->globals[env->globals_size++] = function;
}

struct environment *parser_parse_program(struct parser *parser) {
    struct environment *env = make_environment();

    while (1) {
        struct word *fn = parser_parse_function(parser, env);
        if (!parser_success(parser)) {
            return NULL;
        }

        if (!fn) {
            break;
        }

        environment_add_global(env, fn);
    }

    _Bool foundmain = 0;
    for (size_t i = 0; i < env->globals_size; i++) {
        struct word *word_fn = env->globals[i];
        if (word_fn->function.type == FUNCTION_TYPE_REGULAR &&
            strcmp(word_fn->function.fn->name, "main") == 0) {
            env->entry = word_fn->function.fn;
            foundmain  = 1;
            break;
        }
    }

    if (!foundmain) {
        parser_errorf(parser, "error: could not find entry point main.\n");
    }

    return env;
}

#ifdef ENABLE_FFI
ffi_type *catcat_identifier_to_ffi_type(char const *ccident) {
    if (strcmp(ccident, "pointer") == 0) {
        return &ffi_type_pointer;
    } else if (strcmp(ccident, "int") == 0) {
        return &ffi_type_sint;
    } else {
        fatalf("error: catcat_type_to_ffi_type unsupported type %s.\n",
               ccident);
    }

    return NULL;
}
#endif

struct word *parser_parse_ffi_function(struct parser *parser,
                                       struct environment *env,
                                       char *foreign_function_name) {
#ifdef ENABLE_FFI

    struct token *token;
    tokens_pop(&parser->tokens, &token);

    if (token->type != TOKEN_TYPE_COLON) {
        parser_errorf(
            parser,
            "error: expected colon after ffi definition, but got %s instead.\n",
            token->lexeme);
        goto parser_error_parse_ffi_function;
    }

    GET_NEXT_TOKEN(parser, token);

    if (token->type != TOKEN_TYPE_LITERAL &&
        token->literal.type != LITERAL_TYPE_STRING) {
        parser_errorf(
            parser,
            "error: expected string as first word of ffi definition, but "
            "got %s instead.\n",
            token->lexeme);
        goto parser_error_parse_ffi_function;
    }

    char const *module_name = token->literal.string;
    HMODULE module          = LoadLibraryA(module_name);
    if (!module) {
        parser_errorf(parser, "error: LoadLibraryA failed, %lu\n",
                      GetLastError());
        goto parser_error_parse_ffi_function;
    }

    FARPROC procaddr = GetProcAddress(module, foreign_function_name);

    if (!procaddr)
        fatalf("error: GetProcAddress failed, %lu\n", GetLastError());

    GET_NEXT_TOKEN(parser, token);
    if (token->type != TOKEN_TYPE_IDENTIFIER) {
        parser_errorf(
            parser,
            "error: expected identifier after string in ffi definition, but "
            "got %s instead.\n",
            token->lexeme);
        goto parser_error_parse_ffi_function;
    }

    struct ffi_function *fn = calloc(1, sizeof(*fn));
    fn->ret_type            = *catcat_identifier_to_ffi_type(token->lexeme);
    fn->fn                  = procaddr;
    fn->name                = foreign_function_name;

    GET_NEXT_TOKEN(parser, token);

    while (token && token->type != TOKEN_TYPE_SEMICOLON) {
        if (!token) {
            parser_errorf(
                parser,
                "error: expected end of function ;, but got EOF instead.\n");
            goto parser_error_parse_ffi_function;
        }

        fn->args[fn->nargs++] = catcat_identifier_to_ffi_type(token->lexeme);
        GET_NEXT_TOKEN(parser, token);
    }

    if (!token) {
        parser_errorf(
            parser,
            "error: expected end of function ;, but got EOF instead.\n");
        goto parser_error_parse_ffi_function;
    }

    if (ffi_prep_cif(&fn->cif, FFI_DEFAULT_ABI, fn->nargs, &fn->ret_type,
                     fn->args) != FFI_OK) {
        parser_errorf(parser, "error: ffi_prep_cif failed.\n");
        goto parser_error_parse_ffi_function;
    }

    struct word *word     = calloc(1, sizeof(*word));
    word->type            = WORD_TYPE_FUNCTION;
    word->function.type   = FUNCTION_TYPE_FFI;
    word->function.ffi_fn = fn;
    return word;

parser_error_parse_ffi_function:
    token_destroy(token);
    return NULL;
#else
    fatalf("FFI support not enabled.");
#endif
}

struct word *parser_parse_function(struct parser *parser,
                                   struct environment *env) {
    struct token *token;
    struct word *word;

    tokens_pop(&parser->tokens, &token);
    if (!token)
        return NULL;

    if (token->type != TOKEN_TYPE_IDENTIFIER) {
        parser_errorf(parser,
                      "error: expected function name, but got %s instead.\n",
                      token->lexeme);
        goto parser_error_parse_function;
    }

    if (strlen(token->lexeme) > 4) {
        if (memcmp(token->lexeme, "ffi@", 4) == 0) {
            char *foreign_function_name = strdup(token->lexeme + 4);
            token_destroy(token);

            return parser_parse_ffi_function(parser, env,
                                             foreign_function_name);
        }
    }

    struct function *function = make_function(token->lexeme);
    word                      = make_word_regular_function(function);

    GET_NEXT_TOKEN(parser, token);

    if (token->type != TOKEN_TYPE_COLON) {
        parser_errorf(
            parser,
            "error: expected colon after function name, but got %s instead.\n",
            token->lexeme);
        goto parser_error_parse_function;
    }

    parser_parse_function_body(parser, env, function, 0);

    return word;

parser_error_parse_function:
    token_destroy(token);
    return NULL;
}

void parser_parse_function_body(struct parser *parser, struct environment *env,
                                struct function *function, _Bool islambda) {
    if (!parser_success(parser))
        return;

    struct token *token;

    while (1) {
        struct word *word = NULL;
        tokens_pop(&parser->tokens, &token);

        if (!token) {
            parser_errorf(
                parser,
                "error in function %s: end of tokens, but expected end of "
                "function.\n",
                function->name);
            goto parser_error_parse_function_body;
        }

        switch (token->type) {
        case TOKEN_TYPE_SEMICOLON: {
            goto parser_parse_function_body_return;
        }
        case TOKEN_TYPE_LITERAL: {
            switch (token->literal.type) {
            case LITERAL_TYPE_STRING: {
                word = make_word_string(token->literal.string);
                break;
            }
            case LITERAL_TYPE_INTEGER: {
                word = make_word_integer(token->literal.integer);
                break;
            }
            default: {
                parser_errorf(parser, "error: unspported literal %s.\n",
                              token->lexeme);
                goto parser_error_parse_function_body;
            }
            }
            break;
        }
        case TOKEN_TYPE_IDENTIFIER: {
            _Bool found_internal_function  = 0;
            struct internal_function *infn = &internal_functions[0];
            for (; infn->name; infn++) {
                if (strcmp(infn->name, token->lexeme) == 0) {
                    word = make_word_cfunction(infn->name, infn->function);
                    found_internal_function = 1;
                    break;
                }
            }

            if (!found_internal_function) {
                _Bool found_globalfn = 0;
                for (size_t i = 0; i < env->globals_size; i++) {
                    char *fn_name        = NULL;
                    struct word *word_fn = env->globals[i];
                    switch (word_fn->function.type) {
                    case FUNCTION_TYPE_REGULAR:
                        fn_name = word_fn->function.fn->name;
                        break;
#ifdef ENABLE_FFI
                    case FUNCTION_TYPE_FFI:
                        fn_name = word_fn->function.ffi_fn->name;
                        break;
#endif
                    default:
                        fatalf("error: unknown function type matched\n");
                    }

                    if (strcmp(fn_name, token->lexeme) == 0) {
                        word = calloc(1, sizeof(*word));
                        word_copy(word, env->globals[i]);
                        found_globalfn = 1;
                        break;
                    }
                }

                if (!found_globalfn) {
                    parser_errorf(
                        parser,
                        "error in function %s: found unknown identifier, %s.\n",
                        function->name, token->lexeme);
                    goto parser_error_parse_function_body;
                }
            }
            break;
        }
        case TOKEN_TYPE_LEFT_BRACKET: {
            struct function *lambda = make_function(NULL);
            parser_parse_function_body(parser, env, lambda, 1);

            word         = calloc(1, sizeof(*word));
            word->type   = WORD_TYPE_LAMBDA;
            word->lambda = lambda;
            break;
        }
        case TOKEN_TYPE_RIGHT_BRACKET: {
            if (!islambda) {
                parser_errorf(
                    parser,
                    "error in function %s: found a ending bracket, but not "
                    "parsing a lambda.\n",
                    function->name);
                goto parser_error_parse_function_body;
            }

            goto parser_parse_function_body_return;
        }
        default: {
            parser_errorf(
                parser,
                "error in function %s: unexpected token, %s, while parsing.\n",
                function->name, token->lexeme);
            goto parser_error_parse_function_body;
        }
        }

        if (!word) {
            parser_errorf(parser, "error: unexpectedly recieved NULL word.\n");
            goto parser_error_parse_function_body;
        }
        function_add_word(function, word);
        token_destroy(token);
    }

parser_parse_function_body_return:
parser_error_parse_function_body:
    token_destroy(token);
    return;
}
