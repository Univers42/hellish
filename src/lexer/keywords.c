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

/* Match shell keywords that are exactly the right length and spelling.
   Splitting into two functions keeps each under the 25-line norm limit.
   Part1 handles the if/elif/else/fi/while/until family. */
static t_tt	match_kw_part1(const char *s, int len)
{
	if (len == 2 && kw_eq(s, "if", 2))
		return (TT_IF);
	if (len == 4 && kw_eq(s, "then", 4))
		return (TT_THEN);
	if (len == 4 && kw_eq(s, "elif", 4))
		return (TT_ELIF);
	if (len == 4 && kw_eq(s, "else", 4))
		return (TT_ELSE);
	if (len == 2 && kw_eq(s, "fi", 2))
		return (TT_FI);
	if (len == 5 && kw_eq(s, "while", 5))
		return (TT_WHILE);
	if (len == 5 && kw_eq(s, "until", 5))
		return (TT_UNTIL);
	return (TT_END);
}

/* Part2 handles for/do/done/case/esac and the special single-char keywords
   `{`, `}`, and `!`. Note that `{` and `}` are keywords, not operators;
   the operator table handles `(` and `)`. */
static t_tt	match_kw_part2(const char *s, int len)
{
	if (len == 3 && kw_eq(s, "for", 3))
		return (TT_FOR);
	if (len == 2 && kw_eq(s, "do", 2))
		return (TT_DO);
	if (len == 4 && kw_eq(s, "done", 4))
		return (TT_DONE);
	if (len == 4 && kw_eq(s, "case", 4))
		return (TT_CASE);
	if (len == 4 && kw_eq(s, "esac", 4))
		return (TT_ESAC);
	if (len == 1 && *s == '{')
		return (TT_LBRACE);
	if (len == 1 && *s == '}')
		return (TT_RBRACE);
	if (len == 1 && *s == '!')
		return (TT_BANG);
	if (len == 6 && kw_eq(s, "coproc", 6))
		return (TT_COPROC);
	if (len == 6 && kw_eq(s, "select", 6))
		return (TT_SELECT);
	return (TT_END);
}

/* Return true if tt is a token that puts the parser in "command position":
   the very next word may be a keyword. The full list mirrors the POSIX rule
   that a WORD is a reserved word only when it appears at the start of a
   command or after certain structural tokens (pipe, semicolon, do, etc.). */
bool	is_cmd_position(t_tt tt)
{
	return (tt == TT_SEMICOLON || tt == TT_PIPE || tt == TT_AND
		|| tt == TT_OR || tt == TT_BRACE_LEFT || tt == TT_NEWLINE
		|| tt == TT_AMPERSAND || tt == TT_IF || tt == TT_THEN
		|| tt == TT_ELIF || tt == TT_ELSE || tt == TT_WHILE
		|| tt == TT_UNTIL || tt == TT_DO || tt == TT_LBRACE
		|| tt == TT_BANG || tt == TT_DSEMI || tt == TT_BRACE_RIGHT
		|| tt == TT_COPROC);
}

/* If the token's text matches a reserved word, upgrade its type from TT_WORD
   to the corresponding keyword token type. Called only when the token is in
   command position (determined by reclassify_keywords). */
void	reclassify_word(t_ltoken *t, char *base)
{
	t_tt	kw;

	kw = match_kw_part1(base + t->off, t->len);
	if (kw == TT_END)
		kw = match_kw_part2(base + t->off, t->len);
	if (kw != TT_END)
		t->tt = kw;
}
