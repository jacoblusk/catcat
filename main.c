#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lexer.h"
#include "parser.h"
#include "error.h"

int main(int argc, char **argv) {
	struct lexer lexer;
	lexer_init(&lexer);

	char const *program = "print-one: 1 print ; \n main: 5 [ print-one ] times ;";

	struct token *tokens = lexer_tokenize(&lexer, program);
	struct parser parser = {tokens};
	struct environment *env = parser_parse_program(&parser);
	environment_execute(env);
	free(env);

	return EXIT_SUCCESS;
}