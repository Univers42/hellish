#include "../incs/inlexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* simple ANSI color helpers */
#define C_RESET "\033[0m"
#define C_BOLD  "\033[1m"
#define C_RED   "\033[1;31m"
#define C_GREEN "\033[1;32m"
#define C_YELLOW "\033[1;33m"
#define C_CYAN  "\033[1;36m"
#define C_GREY  "\033[90m"

static const char *token_type_to_str(t_tok_type t)
{
    switch (t) {
    case TOK_WORD: return "WORD";
    case TOK_PIPE: return "PIPE";
    case TOK_REDIR_IN: return "REDIR_IN";
    case TOK_REDIR_OUT: return "REDIR_OUT";
    case TOK_REDIR_APPEND: return "APPEND";
    case TOK_HEREDOC: return "HEREDOC";
    case TOK_DOLLAR: return "DOLLAR";
    default: return "UNKNOWN";
    }
}

static const char *token_color(t_tok_type t)
{
    if (t == TOK_PIPE) return C_CYAN;
    if (t == TOK_REDIR_IN || t == TOK_REDIR_OUT || t == TOK_REDIR_APPEND || t == TOK_HEREDOC) return C_YELLOW;
    if (t == TOK_WORD) return C_GREEN;
    return C_GREY;
}

static void print_header(const char *title, size_t count)
{
    printf("\n%s%s%s  %s(%zu tokens)%s\n", C_BOLD, title, C_RESET, C_GREY, count, C_RESET);
    printf("+-----+------------+-----+--------------------------------+\n");
    printf("| idx | type       | len | lexeme                         |\n");
    printf("+-----+------------+-----+--------------------------------+\n");
}

static void print_footer(void)
{
    printf("+-----+------------+-----+--------------------------------+\n\n");
}

static void print_tokens_vec(t_tok_vec *vec)
{
    if (!vec) { printf("(null vector)\n"); return; }
    print_header("TOKEN LIST", vec->count);
    for (size_t i = 0; i < vec->count; ++i)
    {
        t_toke *tk = vec->tokens[i];
        const char *lex = tk && tk->value.str ? tk->value.str : "";
        const char *tstr = tk ? token_type_to_str(tk->type) : "(null)";
        const char *col = tk ? token_color(tk->type) : C_GREY;
        int len = tk ? (int)strlen(lex) : 0;
        /* print row without breaking modularity */
        printf("| %3zu | %-10s | %3d |", i, tstr, len);
        printf("%s", col);
        /* print lexeme padded to 32 columns */
        for (int j = 0; j < 32; ++j) {
            if (j < len) putchar(lex[j]); else putchar(' ');
        }
        printf("%s|\n", C_RESET);
    }
    print_footer();
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        /* treat argv[1] as a single input string */
        const char *input = argv[1];
        t_tok_vec *v = lexer(input);
        if (!v) { printf("lexer returned NULL\n"); return 1; }
        print_tokens_vec(v);
        vector_free(v);
        return 0;
    }

    printf("Interactive visualizer. Type 'quit' or Ctrl-D to exit.\n");
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    while (1)
    {
        printf("vis> ");
        n = getline(&line, &cap, stdin);
        if (n <= 0) break;
        if (line[n-1] == '\n') line[n-1] = '\0';
        if (strcmp(line, "quit") == 0) break;
        t_tok_vec *v = lexer(line);
        if (!v) { printf("lexer returned NULL (parse error)\n"); continue; }
        print_tokens_vec(v);
        vector_free(v);
    }
    free(line);
    return 0;
}
