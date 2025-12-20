#include "lexer.h"

int is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n');
}

int is_pipe(char c)
{
    return (c == '|');
}

int is_redirect(char c)
{
    return (c == '<' || c == '>');
}

int is_quote(char c)
{
    return (c == '\'' || c == '\"');
}

int is_special_char(char c)
{
    return (is_whitespace(c) || is_pipe(c) || is_redirect(c) || is_quote(c));
}