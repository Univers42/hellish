/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 01:54:58 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/12 03:53:38 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "lexer.h"
# include "libft.h"
# include <stdio.h>
# include <stdlib.h>

static t_hash *get_color_map(void)
{
    static t_hash map;
    static bool ready = false;
    if (ready)
        return &map;
    if (!hash_init(&map, 128))
        return NULL;
    hash_set(&map, "TT_END", (void *)ASCII_BLUE);
    hash_set(&map, "TT_WORD", (void *)ASCII_GREEN);
    hash_set(&map, "TT_REDIRECT_LEFT", (void *)ASCII_YELLOW);
    hash_set(&map, "TT_REDIRECT_RIGHT", (void *)ASCII_YELLOW);
    hash_set(&map, "TT_APPEND", (void *)ASCII_YELLOW);
    hash_set(&map, "TT_HEREDOC", (void *)ASCII_YELLOW);
    hash_set(&map, "TT_PIPE", (void *)ASCII_CYAN);
    hash_set(&map, "TT_OR", (void *)ASCII_CYAN);
    hash_set(&map, "TT_AND", (void *)ASCII_CYAN);
    hash_set(&map, "TT_SEMICOLON", (void *)ASCII_MAGENTA);
    hash_set(&map, "TT_NEWLINE", (void *)ASCII_MAGENTA);
    hash_set(&map, "TT_QWORD", (void *)ASCII_GREEN);
    hash_set(&map, "TT_SQWORD", (void *)ASCII_GREEN);
    hash_set(&map, "TT_DQWORD", (void *)ASCII_GREEN);
    hash_set(&map, "TT_ENVVAR", (void *)ASCII_GREEN);
    hash_set(&map, "TT_DQENVVAR", (void *)ASCII_GREEN);
    ready = true;
    return &map;
}

bool	is_space(char c)
{
	return (c == ' ' || c == '\t');
}

bool	is_special_char(char c)
{
	char *specials = ";$'\"<>|&()\n";
	if (ft_strchr(specials, c) || is_space(c))
		return (true);
	return (false);
}

/* function-scoped singleton for token-name table (no global variables) */
static const char **get_tt_names(void)
{
    static const char *names[256];
    static int inited = 0;
    if (!inited)
    {
        for (size_t i = 0; i < 256; ++i)
            names[i] = "TT_UNKNOWN";
        names[TT_END] = "TT_END";
        names[TT_WORD] = "TT_WORD";
        names[TT_REDIRECT_LEFT] = "TT_REDIRECT_LEFT";
        names[TT_REDIRECT_RIGHT] = "TT_REDIRECT_RIGHT";
        names[TT_APPEND] = "TT_APPEND";
        names[TT_PIPE] = "TT_PIPE";
        names[TT_BRACE_LEFT] = "TT_BRACE_LEFT";
        names[TT_BRACE_RIGHT] = "TT_BRACE_RIGHT";
        names[TT_OR] = "TT_OR";
        names[TT_AND] = "TT_AND";
        names[TT_SEMICOLON] = "TT_SEMICOLON";
        names[TT_HEREDOC] = "TT_HEREDOC";
        names[TT_NEWLINE] = "TT_NEWLINE";
        names[TT_SQWORD] = "TT_SQWORD";
        names[TT_DQWORD] = "TT_DQWORD";
        names[TT_ENVVAR] = "TT_ENVVAR";
        names[TT_DQENVVAR] = "TT_DQENVVAR";
        inited = 1;
    }
    return names;
}

char *tt_to_str(t_tt tt)
{
    const char **names = get_tt_names();
    if ((unsigned)tt < 256)
        return (char *)names[tt];
    return "TT_INVALID";
}

static const char *token_color(t_tt tt)
{
    t_hash *map = get_color_map();
    if (!map)
        return ASCII_BLUE;
    const char *name = tt_to_str(tt);
    const char *c = (const char *)hash_get(map, name);
    if (c)
        return c;
    return ASCII_BLUE;
}

static size_t visible_lexeme_len(t_token *t)
{
    size_t i = 0;
    size_t len = 0;
    while (i < (size_t)t->len)
    {
        unsigned char c = (unsigned char)t->start[i];
        if (c == '\n' || c == '\t')
            len += 2; /* printed as \\n or \\t */
        else
            len += 1;
        i++;
    }
    return (len);
}

static void print_visible_lexeme_noquotes(t_token *t)
{
    size_t i = 0;
    while (i < (size_t)t->len)
    {
        unsigned char c = (unsigned char)t->start[i];
        if (c == '\n')
            ft_printf("\\n");
        else if (c == '\t')
            ft_printf("\\t");
        else
            ft_printf("%c", c);
        i++;
    }
}

static size_t num_digits(size_t v)
{
	size_t d = 1;
	while (v >= 10)
	{
		v /= 10;
		d++;
	}
	return d;
}

/* Compute column widths dynamically for a neat table */
static void compute_columns(t_deque_tt *tokens, size_t *w_name,
    size_t *w_len, size_t *w_lexeme)
{
    size_t i, n;
    t_token *t;
    size_t tmp;

    n = tokens->deqtok.len;
    *w_name = 4; /* at least "type" */
    *w_len = 3;
    *w_lexeme = ft_strlen("(empty)"); /* ensure fits the (empty) label */
    i = 0;
    while (i < n)
    {
        t = (t_token *)deque_idx(&tokens->deqtok, i);
        tmp = ft_strlen(tt_to_str(t->tt));
        if (tmp > *w_name) *w_name = tmp;
        tmp = 0;
        if (t->len > 0)
            tmp = visible_lexeme_len(t) + 2; /* quotes */
        if (tmp > *w_lexeme) *w_lexeme = tmp;
        /* compute width of numeric length */
        tmp = num_digits((size_t)t->len);
        if (tmp > *w_len) *w_len = tmp;
        i++;
    }
}

static void print_table_header(size_t w_name, size_t w_len, size_t w_lexeme)
{
	ft_printf(ASCII_MAGENTA "╔");
	for (size_t i = 0; i < w_name + 2; ++i) ft_printf("═");
	ft_printf("╦");
	for (size_t i = 0; i < w_len + 2; ++i) ft_printf("═");
	ft_printf("╦");
	for (size_t i = 0; i < w_lexeme + 2; ++i) ft_printf("═");
	ft_printf("╗\n" RESET_TERM);

	ft_printf(ASCII_MAGENTA "║ " RESET_TERM BOLD_TERM "%-*s" RESET_TERM " " ASCII_MAGENTA "║ " RESET_TERM BOLD_TERM "%*s" RESET_TERM " " ASCII_MAGENTA "║ " RESET_TERM BOLD_TERM "%-*s" RESET_TERM " " ASCII_MAGENTA "║\n" RESET_TERM,
		(int)w_name, "type", (int)w_len, "len", (int)w_lexeme, "lexeme");

	ft_printf(ASCII_MAGENTA "╠");
	for (size_t i = 0; i < w_name + 2; ++i) ft_printf("═");
	ft_printf("╬");
	for (size_t i = 0; i < w_len + 2; ++i) ft_printf("═");
	ft_printf("╬");
	for (size_t i = 0; i < w_lexeme + 2; ++i) ft_printf("═");
	ft_printf("╣\n" RESET_TERM);
}

static void print_table_footer(size_t w_name, size_t w_len, size_t w_lexeme)
{
	ft_printf(ASCII_MAGENTA "╚");
	for (size_t i = 0; i < w_name + 2; ++i) ft_printf("═");
	ft_printf("╩");
	for (size_t i = 0; i < w_len + 2; ++i) ft_printf("═");
	ft_printf("╩");
	for (size_t i = 0; i < w_lexeme + 2; ++i) ft_printf("═");
	ft_printf("╝\n" RESET_TERM);
}

void print_tokens(t_deque_tt *tokens)
{
	size_t i = 0;
	t_token *curr;
	size_t w_name, w_len, w_lexeme;

	ft_printf("------- PRINTING TOKENS --------\n");
	compute_columns(tokens, &w_name, &w_len, &w_lexeme);
	print_table_header(w_name, w_len, w_lexeme);
	while (i < tokens->deqtok.len)
	{
		curr = (t_token *)deque_idx(&tokens->deqtok, i);
		/* If tokenizer didn't mark quoted words, detect them here for debugging display */
		const char *name = tt_to_str(curr->tt);
		/* detect quoted lexemes: if TT_WORD but starts and ends with matching quotes */
		if (curr->tt == TT_WORD && curr->len >= 2)
		{
			unsigned char fst = ((unsigned char *)curr->start)[0];
			unsigned char lst = ((unsigned char *)curr->start)[curr->len - 1];
			if (fst == '"' && lst == '"')
				name = "TT_DQWORD";
			else if (fst == '\'' && lst == '\'')
				name = "TT_SQWORD";
		}
		const char *color = token_color(curr->tt);
 		/* print row with consistent separators/padding as header */
 		ft_printf(ASCII_MAGENTA "║ " RESET_TERM);
 		/* type column */
 		ft_printf("%s%-*s%s ", color, (int)w_name, name, RESET_TERM);
 		ft_printf(ASCII_MAGENTA "║ " RESET_TERM);
 		/* len column */
 		ft_printf("%*d ", (int)w_len, curr->len);
 		ft_printf(ASCII_MAGENTA "║ " RESET_TERM);
 		/* lexeme column: print visible content then pad */
 		if (curr->len > 0)
 		{
 			ft_printf("%s", ASCII_GREY);
 			print_visible_lexeme_noquotes(curr);
 			ft_printf("%s", RESET_TERM);
 			size_t vis = visible_lexeme_len(curr);
 			if (vis < w_lexeme)
 			{
 				for (size_t _p = 0; _p < w_lexeme - vis; ++_p)
 					ft_printf(" ");
 			}
 		}
 		else
 		{
 			const char *emp = "(empty)";
 			ft_printf("%s%s%s", ASCII_GREY, emp, RESET_TERM);
 			size_t vis = ft_strlen(emp);
 			if (vis < w_lexeme)
 				for (size_t _p = 0; _p < w_lexeme - vis; ++_p) ft_printf(" ");
 		}
 		/* closing column separator */
 		ft_printf(ASCII_MAGENTA " ║" RESET_TERM "\n");
 		i++;
	}
	print_table_footer(w_name, w_len, w_lexeme);
	ft_printf("------- DONE --------\n");
}