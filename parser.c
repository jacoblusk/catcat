#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ffi.h>

#include "parser.h"
#include "error.h"

static struct internal_function internal_functions[] =
{
	{"apply", __applyfunction},
	{"print", __printfunction},
	{"prints",__printsfunction},
	{"dup",   __dupfunction},
	{"swap",  __swapfunction},
	{"rot",   __rotfunction},
	{"bi",    __bifunction},
	{"times", __timesfunction},
	{"+",     __addfunction},
	{"*",     __mulfunction},
	{".",     __dropfunction},
	{"compose", __composefunction},
	{"curry",  __curryfunction},
	{"equal?",  __equalfunction},
	{"putstestffi",  __putstestffifunction},
	{NULL, NULL}
};

struct function *make_function(char const *name) {
	struct function *f = malloc(sizeof(*f));
	f->name = "[lambda]";

	if(name)
		f->name = strdup(name);

	f->words = calloc(NFUNCTION_WORDS, sizeof(struct word));
	f->capacity = NFUNCTION_WORDS;
	f->size = 0;

	return f;
}


struct word *make_word_string(char const *string) {
	struct word *word = NULL;

	word = malloc(sizeof(*word));
	word->type = WORD_TYPE_VALUE;
	word->value.type = WORD_VALUE_TYPE_STRING;
	word->value.string = strdup(string);

	return word;
}

struct word *make_word_integer(uint64_t value) {
	struct word *word = NULL;

	word = malloc(sizeof(*word));
	word->type = WORD_TYPE_VALUE;
	word->value.type = WORD_VALUE_TYPE_INTEGER;
	word->value.integer = value;

	return word;
}

struct word *make_word_cfunction(const char *name, cfunction cfn) {
	struct word *word = NULL;

	word = calloc(1, sizeof(*word));
	word->type = WORD_TYPE_FUNCTION;
	word->function.type = FUNCTION_TYPE_CFUNCTION;
	word->function.cfn.function = cfn;
	word->function.cfn.name = strdup(name);

	return word;
}

struct word *make_word_regular_function(struct function *fn) {
	struct word *word = NULL;

	word = calloc(1, sizeof(*word));
	word->type = WORD_TYPE_FUNCTION;
	word->function.type = FUNCTION_TYPE_REGULAR;
	word->function.fn = fn;

	return word;
}

void environment_add_global(struct environment env[static 1], struct word f[static 1]) {
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
		struct word *fn = parser_parse_function(parser, env);
		if(!fn) {
			break;
		}

		environment_add_global(env, fn);
	}

	_Bool foundmain = 0;
	for(size_t i = 0; i < env->globals_size; i++) {
		struct word *word_fn = env->globals[i];
		if(word_fn->function.type == FUNCTION_TYPE_REGULAR && strcmp(word_fn->function.fn->name, "main") == 0) {
			env->entry = word_fn->function.fn;
			foundmain = 1;
			break;
		}
	}

	if(!foundmain)
		fatalf("error: could not find entry point main.\n");

	return env;
}

struct ffi_type_match_pair {
	char const *catcat_identifier;
	ffi_type ffi_type_match;
};

struct word *parser_parse_ffi_function(struct parser parser[static 1], struct environment env[static 1], const char *foreign_function_name) {
	struct token *token;

	struct ffi_type_match_pair ffi_type_match_pairs[] = {
		{"int", ffi_type_sint},
		{"pointer", ffi_type_pointer},
		{NULL, ffi_type_void}
	};

	tokens_pop(&parser->tokens, &token);

	if(token->type != TOKEN_TYPE_COLON)
		fatalf("error: expected colon after ffi definition, but got %s instead.\n", token->lexeme);
	free(token); tokens_pop(&parser->tokens, &token);

	if(token->type != TOKEN_TYPE_STRING)
		fatalf("error: expected string as first word of ffi definition, but got %s instead.\n", token->lexeme);

	char *module_name = strdup(token->lexeme);
	HMODULE module = LoadLibraryA(module_name);
	if(!module)
		fatalf("error: LoadLibraryA failed, %lu\n", GetLastError());

	FARPROC procaddr = GetProcAddress(module, foreign_function_name);

	if(!procaddr)
		fatalf("error: GetProcAddress failed, %lu\n", GetLastError());

	free(token); tokens_pop(&parser->tokens, &token);
	if(token->type != TOKEN_TYPE_IDENTIFIER)
		fatalf("error: expected identifier after string in ffi definition, but got %s instead.\n", token->lexeme);

	struct ffi_function *fn = calloc(1, sizeof(*fn));
	fn->name = foreign_function_name;
	fn->cfn = procaddr;

	_Bool return_type_found = 0;
	for(int i = 0; ffi_type_match_pairs[i].catcat_identifier; i++) {
		if(strcmp(token->lexeme, ffi_type_match_pairs[i].catcat_identifier) == 0) {
			fn->rtype = ffi_type_match_pairs[i].ffi_type_match;
			return_type_found = 1;
		}
	}

	if(!return_type_found)
		fatalf("error: could not find a valid return type in cffi definition, got %s instead.\n", token->lexeme);

	fn->args = malloc(sizeof(ffi_type *) * 8);
	while(1) {
		tokens_pop(&parser->tokens, &token);
		switch(token->type) {
		case TOKEN_TYPE_IDENTIFIER: {
			_Bool argument_found = 0;
			for(int i = 0; ffi_type_match_pairs[i].catcat_identifier; i++) {
				if(strcmp(token->lexeme, ffi_type_match_pairs[i].catcat_identifier) == 0) {

					fn->args[fn->args_size++] = &(ffi_type_match_pairs[i].ffi_type_match);
					argument_found = 1;
				}
			}

			if(!argument_found)
				fatalf("error: expected a valid argument type, got %s instead.\n", token->lexeme);
			break;
		}
		case TOKEN_TYPE_SEMICOLON:
			free(token);
			goto finish_definition;
			break;
		default:
			fatalf("error: expected an identifier or end of definition, got %s instead.\n", token->lexeme);
		}

		free(token);
	}

finish_definition:

	printf("args_size: %llu\n",fn->args_size);
	if(ffi_prep_cif(&fn->cfn, FFI_DEFAULT_ABI, fn->args_size, &fn->rtype, fn->args) != FFI_OK)
		fatalf("error: ffi_prep_cif failed.\n");

	void *values[1];
	char *s = "Hello from Parser!";
	ffi_arg rc;
  
  	values[0] = &s;
	ffi_call(&fn->cif, FFI_FN(fn->cfn), &rc, values);

	struct word *word = calloc(1, sizeof(*word));
	word->type = WORD_TYPE_FUNCTION;
	word->function.type = FUNCTION_TYPE_FFI;
	word->function.ffi_fn = fn;
	return word;
}

struct word *parser_parse_function(struct parser parser[static 1], struct environment env[static 1]) {
	struct token *token;
	struct word *word;

	tokens_pop(&parser->tokens, &token);
	if(!token)
		return NULL;

	if(token->type != TOKEN_TYPE_IDENTIFIER)
		fatalf("error: expected function name, but got %s instead.\n", token->lexeme);


	if(strlen(token->lexeme) > 4) {
		if(memcmp(token->lexeme, "ffi@", 4) == 0) {
			char *foreign_function_name = strdup(token->lexeme + 4);
			free(token);

			return parser_parse_ffi_function(parser, env, foreign_function_name);
		}
	}

	struct function *function = make_function(token->lexeme);
	word = make_word_regular_function(function);

	free(token); tokens_pop(&parser->tokens, &token);

	if(token->type != TOKEN_TYPE_COLON)
		fatalf("error: expected colon after function name, but got %s instead.\n", token->lexeme);

	parser_parse_function_body(parser, env, function, 0);

	return word;
}

void parser_parse_function_body(struct parser parser[static 1], struct environment env[static 1],
								struct function *function, _Bool islambda) {
	struct token *token;

	while(1) {
		struct word *word = NULL;
		tokens_pop(&parser->tokens, &token);

		if(!token) {
			fatalf("error in function %s: end of tokens, but expected end of function.\n", function->name);
		}

		switch(token->type) {
			case TOKEN_TYPE_SEMICOLON: {
				free(token);
				return;
				break;
			}
			case TOKEN_TYPE_STRING: {
				word = make_word_string(token->literal.string);
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
						word = make_word_cfunction(infn->name, infn->function);
						found_internal_function = 1;
						break;
					}
				}
				
				if(!found_internal_function) {
					_Bool found_globalfn = 0;
					for(size_t i = 0; i < env->globals_size; i++) {
						char *fn_name = NULL;
						struct word *word_fn = env->globals[i];
						switch(word_fn->function.type) {
						case FUNCTION_TYPE_REGULAR:
							fn_name = word_fn->function.fn->name;
							break;
						case FUNCTION_TYPE_FFI:
							fn_name = word_fn->function.ffi_fn->name;
							break;
						default:
							fatalf("error: unknown function type matched\n");
						}

						if(strcmp(fn_name, token->lexeme) == 0) {
							word = calloc(1, sizeof(*word));
							word_copy(word, env->globals[i]);
							found_globalfn = 1;
							break;
						}
					}

					if(!found_globalfn)
						fatalf("error in function %s: found unknown identifier, %s.\n", function->name, token->lexeme);
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
					fatalf("error in function %s: found a ending bracket, but not parsing a lambda.\n", function->name);
				}
				
				free(token);
				return;
			}
			default: {
				fatalf("error in function %s: unexpected token, %s, while parsing.\n", function->name, token->lexeme);
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
