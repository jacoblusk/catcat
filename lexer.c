#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "lexer.h"

struct token *token_make(enum token_type type, struct cursor *cursor,
                         char const *lexeme) {
    struct token *token = calloc(1, sizeof(*token));
    if (!token)
        return NULL;

    token->type = type;

    memcpy(&token->cursor, cursor, sizeof(*cursor));
    token->lexeme = lexeme;
    return token;
}

void token_destroy(struct token *token) {
    free(token->lexeme);

    if (token->type == TOKEN_TYPE_LITERAL) {
        switch (token->type) {
        case LITERAL_TYPE_STRING:
            free(token->literal.string);
            break;
        }
    }

    memset(token, 0, sizeof(*token));
    free(token);
}

struct token *token_make_syntax(char c, struct cursor *cursor) {
    struct token *token = calloc(1, sizeof(*token));
    if (!token)
        return NULL;

    enum token_type type;

    switch (c) {
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
    case '-':
        type = TOKEN_TYPE_IDENTIFIER;
        break;
    default:
        fatalf("recieved non-syntax token in token_make_syntax '%c'\n", c);
    }

    token->type = type;
    memcpy(&token->cursor, cursor, sizeof(*cursor));

    char *lexeme = malloc(sizeof(char) * 2);
    lexeme[0]    = c;
    lexeme[1]    = '\0';

    token->lexeme = lexeme;
    return token;
}

void tokens_prepend(struct token **tokens, struct token *token) {
    struct token *head = *tokens;
    token->next        = head;
    *tokens            = token;
}

void tokens_reverse(struct token **tokens) {
    struct token *current = *tokens;
    struct token *prev = NULL, *next = NULL;

    while (current != NULL) {
        next          = current->next;
        current->next = prev;

        prev    = current;
        current = next;
    }

    *tokens = prev;
}

void tokens_pop(struct token **tokens, struct token **out) {
    struct token *head = *tokens;

    if (!head) {
        *out = NULL;
    } else {
        *tokens = head->next;
        *out    = head;
    }
}

char lexer_advance(struct lexer *lexer) {
    char c = *lexer->source;
    if (c == '\n') {
        lexer->cursor.line++;
        lexer->cursor.column = 0;
    } else {
        lexer->cursor.column++;
    }

    if (c != '\0') {
        lexer->source++;
    }

    return c;
}

char lexer_peek(struct lexer *lexer) { return *lexer->source; }

void lexer_init(struct lexer *lexer) { memset(lexer, 0, sizeof(*lexer)); }

_Bool isvalid_ident(char c) {
    return isalpha(c) || isdigit(c) || c == '-' || c == '?' || c == '!' ||
           c == '@' || c == '*';
}

struct token *lexer_lex_identifier(struct lexer *lexer) {
    size_t length        = 0;
    struct cursor cursor = lexer->cursor;
    char c;

    while (isvalid_ident(c = lexer_peek(lexer))) {
        length++;
        lexer_advance(lexer);
    }

    char *identifier = malloc(sizeof(char) * (length + 1));
    if (!identifier)
        return NULL;

    identifier[length] = '\0';

    memcpy(identifier, lexer->source - length, sizeof(char) * length);

    struct token *token =
        token_make(TOKEN_TYPE_IDENTIFIER, &cursor, identifier);
    return token;
}

struct token *lexer_lex_string(struct lexer *lexer) {
    size_t length        = 0;
    struct cursor cursor = lexer->cursor;
    char c;

    lexer_advance(lexer);

    while ((c = lexer_peek(lexer)) != '"') {
        length++;
        lexer_advance(lexer);
    }

    lexer_advance(lexer);

    char *string   = malloc(sizeof(char) * (length + 1));
    string[length] = '\0';

    memcpy(string, lexer->source - length - 1, sizeof(char) * length);

    struct token *token   = token_make(TOKEN_TYPE_LITERAL, &cursor, string);
    token->literal.type   = LITERAL_TYPE_STRING;
    token->literal.string = string;

    return token;
}

struct token *lexer_lex_number(struct lexer *lexer) {
    size_t length        = 0;
    struct cursor cursor = lexer->cursor;
    char c;

    while (isdigit(c = lexer_peek(lexer))) {
        length++;
        lexer_advance(lexer);
    }

    char *number   = malloc(sizeof(char) * (length + 1));
    number[length] = '\0';

    memcpy(number, lexer->source - length, sizeof(char) * length);

    struct token *token    = token_make(TOKEN_TYPE_LITERAL, &cursor, number);
    token->literal.type    = LITERAL_TYPE_INTEGER;
    token->literal.integer = atoi(number);

    return token;
}

struct token *lexer_tokenize(struct lexer *lexer, char const *source) {
    char c;
    lexer->source        = source;
    struct token *tokens = NULL;

    while ((c = lexer_peek(lexer)) != '\0') {
        struct token *token = NULL;

        if (isspace(c)) {
            lexer_advance(lexer);
            continue;
        }

        switch (c) {
        case '[':
        case ']':
        case ':':
        case ';':
        case '+':
        case '*':
        case '.':
            token = token_make_syntax(c, &lexer->cursor);
            lexer_advance(lexer);
            goto add_token_to_list;
        case '"':
            token = lexer_lex_string(lexer);
        }

        if (isalpha(c)) {
            token = lexer_lex_identifier(lexer);
        } else if (isdigit(c)) {
            token = lexer_lex_number(lexer);
        }

    add_token_to_list:
        tokens_prepend(&tokens, token);
    }

    tokens_reverse(&tokens);
    return tokens;
}