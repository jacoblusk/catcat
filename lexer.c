#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "error.h"

struct token *token_make_token(enum token_type type, struct cursor *cursor, char const lexeme[static 1]) {
	struct token *token = calloc(1, sizeof(*token));
	token->type = type;

	memcpy(&token->cursor, cursor, sizeof(*cursor));
	token->lexeme = lexeme;
	return token;
}

struct token *token_make_syntax(char c, struct cursor cursor[static 1]) {
	struct token *token = calloc(1, sizeof(*token));
	enum token_type type;

	switch(c) {
	case '[':
		type = TOKEN_TYPE_LEFT_BRACKET;
		break;
	case ']':
		type = TOKEN_TYPE_RIGHT_BRACKET;
		break;
	case ':':
		type = TOKEN_TYPE_COLON;
		break;
	case ';':
		type = TOKEN_TYPE_SEMICOLON;
		break;
	case '.':
	case '+':
	case '*':
		type = TOKEN_TYPE_IDENTIFIER;
		break;
	default:
		fatalf("recieved non-syntax token in token_make_syntax '%c'\n", c);
	}

	token->type = type;
	memcpy(&token->cursor, cursor, sizeof(*cursor));

	char *lexeme = malloc(sizeof(char) * 2);
	lexeme[0] = c;
	lexeme[1] = '\0';

	token->lexeme = lexeme;
	return token;
}

void tokens_prepend(struct token *tokens[static 1], struct token token[static 1]) {
	struct token *head = *tokens;
	token->next = head;
	*tokens = token; 
}

void tokens_reverse(struct token *tokens[static 1]) {
	struct token *current = *tokens;
	struct token *prev = NULL, *next = NULL;

	while(current != NULL) {
		next = current->next;
		current->next = prev;

		prev = current;
		current = next;
	}

	*tokens = prev;
}

void tokens_pop(struct token *tokens[static 1], struct token *out[static 1]) {
	struct token *head = *tokens;

	if(!head) {
		*out = NULL;
	} else {
		*tokens = head->next;
		*out = head;
	}
}

char lexer_advance(struct lexer l[static 1]) {
	char c = *l->source;
	if(c == '\n') {
		l->cursor.line++;
		l->cursor.column = 0;
	} else {
		l->cursor.column++;
	}

	if(c != '\0') {
		l->source++;
	}

	return c;
}

char lexer_peek(struct lexer l[static 1]) {
	return *l->source;
}

void lexer_init(struct lexer l[static 1]) {
	memset(l, 0, sizeof(*l));
}

_Bool isvalid_ident(char c) {
	return isalpha(c) || isdigit(c) || c == '-' || c =='?' || c == '!';
}

struct token *lexer_lex_identifier(struct lexer l[static 1]) {
	size_t length = 0;
	struct cursor cursor = l->cursor;
	char c;

	while(isvalid_ident(c = lexer_peek(l))) {
		length++;
		lexer_advance(l);
	}

	char *identifier = malloc(sizeof(char) * (length + 1));
	identifier[length] = '\0';

	memcpy(identifier, l->source - length, sizeof(char) * length);

	struct token *token = token_make_token(TOKEN_TYPE_IDENTIFIER, &cursor, identifier);
	return token;
}

struct token *lexer_lex_number(struct lexer l[static 1]) {
	size_t length = 0;
	struct cursor cursor = l->cursor;
	char c;

	while(isdigit(c = lexer_peek(l))) {
		length++;
		lexer_advance(l);
	}

	char *number = malloc(sizeof(char) * (length + 1));
	number[length] = '\0';

	memcpy(number, l->source - length, sizeof(char) * length);

	struct token *token = token_make_token(TOKEN_TYPE_NUMBER, &cursor, number);
	token->literal.type = LITERAL_TYPE_INTEGER;
	token->literal.integer = atoi(number);

	return token;
}

struct token *lexer_tokenize(struct lexer l[static 1], 
		char const source[static 1]) {
	char c;
	l->source = source;
	struct token *tokens = NULL;

	while((c = lexer_peek(l)) != '\0') {
		struct token *token = NULL;

		if(isspace(c)) {
			lexer_advance(l);
			continue;
		}

		switch(c) {
		case '[':
		case ']':
		case ':':
		case ';':
		case '+':
		case '*':
		case '.':
			token = token_make_syntax(c, &l->cursor);
			lexer_advance(l);
			goto add_token_to_list;
		}

		if(isalpha(c)) {
			token = lexer_lex_identifier(l);
		} else if(isdigit(c)) {
			token = lexer_lex_number(l);
		}

	add_token_to_list:
		tokens_prepend(&tokens, token);
	}

	tokens_reverse(&tokens);
	return tokens;
}