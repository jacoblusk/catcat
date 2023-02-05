#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdlib.h>

enum token_type {
    TOKEN_TYPE_LEFT_BRACKET,
    TOKEN_TYPE_RIGHT_BRACKET,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_LITERAL
};

enum literal_type {
    LITERAL_TYPE_STRING,
    LITERAL_TYPE_INTEGER,
    LITERAL_TYPE_FLOAT,
    LITERAL_TYPE_CHARACTER
};

struct literal {
    enum literal_type type;
    union {
        char const *string;
        int64_t integer;
        float floating_point;
        char character;
    };
};

struct cursor {
    size_t line;
    size_t column;
};

struct token {
    enum token_type type;
    char const *lexeme;
    struct literal literal;
    struct cursor cursor;

    struct token *next;
};

struct lexer {
    char const *source;
    struct cursor cursor;
};

void token_destroy(struct token *token);
struct token *token_make(enum token_type type, struct cursor *cursor,
                         char const *lexeme);
struct token *token_make_syntax(char c, struct cursor *cursor);

void tokens_prepend(struct token **tokens, struct token *token);
void tokens_reverse(struct token **tokens);
void tokens_pop(struct token **tokens, struct token **out);

char lexer_advance(struct lexer *lexer);
char lexer_peek(struct lexer *lexer);
void lexer_init(struct lexer *lexer);
struct token *lexer_lex_identifier(struct lexer *lexer);
struct token *lexer_lex_number(struct lexer *lexer);
struct token *lexer_tokenize(struct lexer *lexer, char const *source);

#endif