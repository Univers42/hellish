/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 01:54:58 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/12 02:14:23 by dlesieur         ###   ########.fr       */
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

char	*tt_to_str_p2(t_tt tt)
{
	if (tt == TT_END)
		return ("TT_END");
	if (tt == TT_WORD)
		return ("TT_WORD");
	if (tt == TT_REDIRECT_LEFT)
		return ("TT_REDIRECT_LEFT");
	if (tt == TT_REDIRECT_RIGHT)
		return ("TT_REDIRECT_RIGHT");
	if (tt == TT_APPEND)
		return ("TT_APPEND");
	if (tt == TT_PIPE)
		return ("TT_PIPE");
	if (tt == TT_BRACE_LEFT)
		return ("TT_BRACE_LEFT");
	if (tt == TT_BRACE_RIGHT)
		return ("TT_BRACE_RIGHT");
	if (tt == TT_OR)
		return ("TT_OR");
	if (tt == TT_AND)
		return ("TT_AND");
	ft_assert(false);
	return (0);
}

char	*tt_to_str(t_tt tt)
{
	if (tt == TT_SEMICOLON)
		return ("TT_SEMICOLON");
	if (tt == TT_HEREDOC)
		return ("TT_HEREDOC");
	if (tt == TT_NEWLINE)
		return ("TT_NEWLINE");
	if (tt == TT_SQWORD)
		return ("TT_QWORD");
	if (tt == TT_DQWORD)
		return ("TT_DQWORD");
	if (tt == TT_ENVVAR)
		return ("TT_ENVVAR");
	if (tt == TT_DQENVVAR)
		return ("TT_DQENVVAR");
	return (tt_to_str_p2(tt));
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
		const char *name = tt_to_str(curr->tt);
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