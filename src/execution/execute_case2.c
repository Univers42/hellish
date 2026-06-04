/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_case2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* A subtoken whose glob metacharacters must be matched literally: single- or
** double-quoted text, or the value of a double-quoted variable. */
bool	is_quoted_tok(t_tt tt)
{
	return (tt == TT_SQWORD || tt == TT_DQWORD || tt == TT_DQENVVAR);
}

/* Append a pattern subtoken, backslash-escaping glob metacharacters that
** came from a quoted segment so case_match treats them as literals (POSIX:
** quoted chars in a case pattern lose their special meaning). */
void	append_pat_tok(t_string *s, t_token t)
{
	int		i;
	bool	q;

	if (!t.start)
		return ;
	q = is_quoted_tok(t.tt);
	i = 0;
	while (i < t.len)
	{
		if (q && ft_strchr("*?[\\", t.start[i]))
			vec_push_char(s, '\\');
		vec_push_char(s, t.start[i]);
		i++;
	}
}
