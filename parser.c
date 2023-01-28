#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "parser.h"
#include "error.h"

static struct internal_function internal_functions[] =
{
	{"apply", __applyfunction},
	{"print", __printfunction},
	{"dup",   __dupfunction},
	{"+",     __addfunction},
	{"*",     __mulfunction},
	{".",     __dropfunction},
	{NULL, NULL}
};

struct function *make_function(char const *name) {
	struct function *f = malloc(sizeof(*f));
	f->name = NULL;

	if(name)
		f->name = strdup(name);

	f->words = calloc(NFUNCTION_WORDS, sizeof(struct word));
	f->capacity = NFUNCTION_WORDS;
	f->size = 0;

	return f;
}

struct word *make_word_integer(uint64_t value) {
	struct word *word = NULL;

	word = malloc(sizeof(*word));
	word->type = WORD_TYPE_VALUE;
	word->value.type = WORD_VALUE_TYPE_INTEGER;
	word->value.integer = value;

	return word;
}

struct word *make_word_cfunction(cfunction cfn) {
	struct word *word = NULL;

	word = calloc(1, sizeof(*word));
	word->type = WORD_TYPE_FUNCTION;
	word->function.is_cfunction = 1;
	word->function.cfunction = cfn;

	return word;
}

struct word *make_word_function(struct function *fn) {
	struct word *word = NULL;

	word = calloc(1, sizeof(*word));
	word->type = WORD_TYPE_FUNCTION;
	word->function.is_cfunction = 0;
	word->function.function = fn;

	return word;
}


void function_add_word(struct function f[static 1], struct word w[static 1]) {
	if(f->capacity <= (f->size + 1)) {
		f->capacity++;
		f->capacity *= 2;
		f->words = realloc(f->words, sizeof(struct word *) * f->capacity);
	}

	f->words[f->size++] = w;
}

void environment_add_global(struct environment env[static 1], struct function f[static 1]) {
	if(env->globals_capacity <= (env->globals_size + 1)) {
		env->globals_capacity++;
		env->globals_capacity *= 2;
		env->globals = realloc(env->globals, sizeof(struct function *) * env->globals_capacity);
	}

	env->globals[env->globals_size++] = f;
}

struct environment *parser_parse_program(struct parser parser[static 1]) {
	struct environment *env = make_environment();

	while(1) {
		struct function *fn = parser_parse_function(parser, env);
		if(!fn) {
			break;
		}

		environment_add_global(env, fn);
	}

	_Bool foundmain = 0;
	for(size_t i = 0; i < env->globals_size; i++) {
		struct function *fn = env->globals[i];
		if(strcmp(fn->name, "main") == 0) {
			env->entry = fn;
			foundmain = 1;
			break;
		}
	}

	if(!foundmain)
		fatalf("error: could not find entry point main.");

	return env;
}

struct function *parser_parse_function(struct parser parser[static 1], struct environment env[static 1]) {
	struct token *token;

	tokens_pop(&parser->tokens, &token);
	if(!token)
		return NULL;

	if(token->type != TOKEN_TYPE_IDENTIFIER)
		fatalf("error: expected function name, but got %s instead.", token->lexeme);

	struct function *function = make_function(token->lexeme);

	free(token); tokens_pop(&parser->tokens, &token);

	if(token->type != TOKEN_TYPE_COLON)
		fatalf("error: expected colon after function name, but got %s instead.", token->lexeme);

	parser_parse_function_body(parser, env, function, 0);

	return function;
}

void parser_parse_function_body(struct parser parser[static 1], struct environment env[static 1],
								struct function *function, _Bool islambda) {
	struct token *token;

	while(1) {
		struct word *word = NULL;
		tokens_pop(&parser->tokens, &token);

		if(!token) {
			fatalf("error: end of tokens, but expected end of function.\n");
		}

		switch(token->type) {
			case TOKEN_TYPE_SEMICOLON: {
				free(token);
				return;
				break;
			}
			case TOKEN_TYPE_NUMBER: {
				word = make_word_integer(token->literal.integer);
				break;
			}
			case TOKEN_TYPE_IDENTIFIER: {
				_Bool found_internal_function = 0;
				struct internal_function *infn = &internal_functions[0];
				for(; infn->name; infn++) {
					if(strcmp(infn->name, token->lexeme) == 0) {
						found_internal_function = 1;
						break;
					}
				}
				
				if(found_internal_function) {
					word = make_word_cfunction(infn->function);
				}
				else {
					_Bool found_globalfn = 0;
					for(size_t i = 0; i < env->globals_size; i++) {
						struct function *fn = env->globals[i];
						if(strcmp(fn->name, token->lexeme) == 0) {
							word = make_word_function(fn);
							found_globalfn = 1;
							break;
						}
					}

					if(!found_globalfn)
						fatalf("error: found unknown identifier, %s\n", token->lexeme);
				}
				break;
			}
			case TOKEN_TYPE_LEFT_BRACKET: {
				struct function *lambda = make_function(NULL);
				parser_parse_function_body(parser, env, lambda, 1);

				word = calloc(1, sizeof(*word));
				word->type = WORD_TYPE_LAMBDA;
				word->lambda = lambda;
				break;
			}
			case TOKEN_TYPE_RIGHT_BRACKET: {
				if(!islambda) {
					fatalf("error: found a ending bracket, but not parsing a lambda.\n");
				}
				
				free(token);
				return;
			}
			default: {
				fatalf("error: unexpected token while parsing\n");
			}
		}

		free(token);
		if(word) {
			function_add_word(function, word);
		} else {
			fatalf("error: unexpectedly recieved NULL word.\n");
		}
	}
}
