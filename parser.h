#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdlib.h>

#include "lexer.h"
#include "kernel.h"

#define NFUNCTION_WORDS 256

struct parser {
	struct token *tokens;
};

struct environment *parser_parse_program(struct parser parser[static 1]);
struct word *parser_parse_function(struct parser parser[static 1], struct environment *env);
void parser_parse_function_body(struct parser parser[static 1], struct environment *env,
								struct function *function, _Bool islambda);

#endif