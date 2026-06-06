/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser4.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:51 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Handle a backslash inside a double-quoted context (called from the dquote
   scanner for the older non-reparser path). If the next char is one of the
   four POSIX active-escape chars (", $, \) or a backtick, emit the pair as
   TT_SQWORD (literal, no further processing). Otherwise emit the backslash
   alone (the next char will be picked up by the surrounding scanner). */
void	reparse_dq_bs(t_ast_node *ret, int *i, t_token t)
{
	ft_assert(t.start[*i] == '\\');
	(*i)++;
	if (ft_strchr("\"$\\", t.start[*i]))
		push_subtoken_node(ret, t, create_interval(*i - 1, *i + 1), TT_SQWORD);
	else
		push_subtoken_node(ret, t, create_interval(*i - 1, *i), TT_SQWORD);
	(*i)++;
}

/* If the character at *i is one of the single-character POSIX special
   variables ($? $$ $# $@ $* $! $- $0-$9), consume it and push a subtoken of
   type tt. Returns false for anything else so the caller knows to try the
   plain-ident path next. This is deliberately a separate function rather than
   part of the brace handler so we can unit-test the special-var set easily. */
bool	reparse_special_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt)
{
	int		prev_start;
	char	c;

	prev_start = *i;
	c = t.start[*i];
	if (c != '?' && c != '$' && c != '#' && c != '@' && c != '*'
		&& c != '!' && c != '-' && !ft_isdigit(c))
		return (false);
	(*i)++;
	push_subtoken_node(ret, t, create_interval(prev_start, *i), tt);
	return (true);
}
