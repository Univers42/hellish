#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    TOKEN_WORD,
    TOKEN_OPERATOR
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenList;

void add_token(TokenList *list, TokenType type, const char *start, size_t length) {
    if (length == 0) return;
    if (list->count >= list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 8;
        list->items = realloc(list->items, list->capacity * sizeof(Token));
    }
    Token *t = &list->items[list->count++];
    t->type = type;
    t->value = strndup(start, length);
}

TokenList tokenize(const char *input) {
    TokenList tokens = {0};
    const char *p = input;
    const char *current = NULL;
    enum { NORMAL, SINGLE_QUOTE, DOUBLE_QUOTE } state = NORMAL;

    while (*p) {
        char ch = *p;

        // Handle quoting states
        if (state == SINGLE_QUOTE) {
            if (ch == '\'') state = NORMAL;
            else if (!current) current = p;
            p++;
            continue;
        }

        if (state == DOUBLE_QUOTE) {
            if (ch == '"') {
                state = NORMAL;
            } else if (ch == '\\') {
                p++;
                if (*p && strchr("\\\"`$", *p)) {
                    // keep the escaped char
                }
            }
            p++;
            continue;
        }

        // Whitespace ends a token
        if (isspace(ch)) {
            if (current) {
                add_token(&tokens, TOKEN_WORD, current, p - current);
                current = NULL;
            }
            p++;
            continue;
        }

        // Multi-char operators
        if (!strncmp(p, "||", 2) || !strncmp(p, "&&", 2) ||
            !strncmp(p, "<<", 2) || !strncmp(p, ">>", 2)) {
            if (current) {
                add_token(&tokens, TOKEN_WORD, current, p - current);
                current = NULL;
            }
            add_token(&tokens, TOKEN_OPERATOR, p, 2);
            p += 2;
            continue;
        }

        // Single-char operators
        if (strchr("|;><&()", ch)) {
            if (current) {
                add_token(&tokens, TOKEN_WORD, current, p - current);
                current = NULL;
            }
            add_token(&tokens, TOKEN_OPERATOR, p, 1);
            p++;
            continue;
        }

        // Quoting starts
        if (ch == '\'') {
            state = SINGLE_QUOTE;
            p++;
            continue;
        }

        if (ch == '"') {
            state = DOUBLE_QUOTE;
            p++;
            continue;
        }

        // Comments
        if (ch == '#') {
            while (*p && *p != '\n') p++;
            continue;
        }

        // Normal word
        if (!current) current = p;
        p++;
    }

    if (current) add_token(&tokens, TOKEN_WORD, current, p - current);
    return tokens;
}

void free_tokens(TokenList *tokens) {
    for (size_t i = 0; i < tokens->count; i++)
        free(tokens->items[i].value);
    free(tokens->items);
}

int main(int argc, char **argv)
{
    (void)argc;
    //const char *input = "cat <<EOF | grep \"foo bar\" # comment";
    TokenList tokens = tokenize(argv[1]);

    for (size_t i = 0; i < tokens.count; i++) {
        printf("[%s] \"%s\"\n",
               tokens.items[i].type == TOKEN_WORD ? "WORD" : "OPERATOR",
               tokens.items[i].value);
    }

    free_tokens(&tokens);
    return 0;
}
