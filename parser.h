#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdlib.h>

#include "kernel.h"
#include "lexer.h"

#define NFUNCTION_WORDS 256

struct function;

struct parser_error {
    char *message;
    _Bool needfree;
};

struct parser {
    struct token *tokens;
    struct parser_error error;
};

struct environment *parser_parse_program(struct parser *parser);
struct word *parser_parse_function(struct parser *parser,
                                   struct environment *env);
void parser_parse_function_body(struct parser *parser, struct environment *env,
                                struct function *function, _Bool islambda);

#endif