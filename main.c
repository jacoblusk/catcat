#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lexer.h"
#include "parser.h"
#include "error.h"

int main(int argc, char **argv) {
	FILE *fp;
	char buffer[1024] = {0};

	struct lexer lexer;
	lexer_init(&lexer);

	if(argc < 2)
		fatalf("error: no input file specified.\n");

	fp = fopen(argv[1], "r");
	if(!fp)
		fatalf("error: opening file %s.\n", argv[1]);

	fread(buffer, sizeof(buffer), 1, fp);
	char const *program = &buffer[0];

	struct token *tokens = lexer_tokenize(&lexer, program);
	struct parser parser = {tokens};
	struct environment *env = parser_parse_program(&parser);
	environment_execute(env);
	free(env);

	return EXIT_SUCCESS;
}