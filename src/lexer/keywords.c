/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   keywords.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

bool	is_redirect(t_tt tt);

static t_tt	match_kw_part1(const char *s, int len)
{
	if (len == 2 && ft_strncmp(s, "if", 2) == 0)
		return (TT_IF);
	if (len == 4 && ft_strncmp(s, "then", 4) == 0)
		return (TT_THEN);
	if (len == 4 && ft_strncmp(s, "elif", 4) == 0)
		return (TT_ELIF);
	if (len == 4 && ft_strncmp(s, "else", 4) == 0)
		return (TT_ELSE);
	if (len == 2 && ft_strncmp(s, "fi", 2) == 0)
		return (TT_FI);
	if (len == 5 && ft_strncmp(s, "while", 5) == 0)
		return (TT_WHILE);
	if (len == 5 && ft_strncmp(s, "until", 5) == 0)
		return (TT_UNTIL);
	return (TT_END);
}

static t_tt	match_kw_part2(const char *s, int len)
{
	if (len == 3 && ft_strncmp(s, "for", 3) == 0)
		return (TT_FOR);
	if (len == 2 && ft_strncmp(s, "do", 2) == 0)
		return (TT_DO);
	if (len == 4 && ft_strncmp(s, "done", 4) == 0)
		return (TT_DONE);
	if (len == 4 && ft_strncmp(s, "case", 4) == 0)
		return (TT_CASE);
	if (len == 4 && ft_strncmp(s, "esac", 4) == 0)
		return (TT_ESAC);
	if (len == 1 && *s == '{')
		return (TT_LBRACE);
	if (len == 1 && *s == '}')
		return (TT_RBRACE);
	if (len == 1 && *s == '!')
		return (TT_BANG);
	return (TT_END);
}

bool	is_cmd_position(t_tt tt)
{
	return (tt == TT_SEMICOLON || tt == TT_PIPE || tt == TT_AND
		|| tt == TT_OR || tt == TT_BRACE_LEFT || tt == TT_NEWLINE
		|| tt == TT_AMPERSAND || tt == TT_IF || tt == TT_THEN
		|| tt == TT_ELIF || tt == TT_ELSE || tt == TT_WHILE
		|| tt == TT_UNTIL || tt == TT_DO || tt == TT_LBRACE
		|| tt == TT_BANG || tt == TT_DSEMI || tt == TT_BRACE_RIGHT);
}

void	reclassify_word(t_token *t)
{
	t_tt	kw;

	kw = match_kw_part1(t->start, t->len);
	if (kw == TT_END)
		kw = match_kw_part2(t->start, t->len);
	if (kw != TT_END)
		t->tt = kw;
}
