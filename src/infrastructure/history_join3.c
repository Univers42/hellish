/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_join3.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include "libft.h"

/* A backslash pair scans atomically so \" never toggles quote state. At the
   top level a \<newline> continuation is deleted outright (bash records
   `echo C\` + `D` as `echo CD`); inside quotes or substitutions both bytes
   stay literal, matching what bash keeps there. */
void	hj_copy_escaped(t_hjoin *h)
{
	if (h->s[h->i + 1] == '\n')
	{
		if (!h->dq && !h->btick && h->csub == 0 && h->arith == 0)
		{
			h->i += 2;
			return ;
		}
		vec_push_char(&h->out, '\\');
		vec_push_char(&h->out, '\n');
		h->i += 2;
		return ;
	}
	vec_push_char(&h->out, '\\');
	if (h->s[h->i + 1])
	{
		vec_push_char(&h->out, h->s[h->i + 1]);
		h->i += 2;
	}
	else
		h->i += 1;
}

/* True when the joined text ends with a dangling reserved word (`then`,
   `do`, `in`, ...) after which a ";" would be a syntax error, so the
   boundary must be joined with a plain space instead. */
bool	hj_last_word_kw(const char *s, size_t n)
{
	static const char	*kw[] = {"if", "then", "else", "elif", "while",
		"until", "for", "case", "select", "in", "do", "time", "function",
		"coproc", "!", "[[", "{", NULL};
	size_t				k;
	size_t				w;

	k = n;
	while (k > 0 && !ft_strchr(" \t;&|(){}", s[k - 1]))
		k--;
	if (n - k == 0 || n - k > 8)
		return (false);
	w = 0;
	while (kw[w])
	{
		if (ft_strlen(kw[w]) == n - k && !ft_strncmp(kw[w], s + k, n - k))
			return (true);
		w++;
	}
	return (false);
}

/* Nested parens inside $( ) or $(( )): the arithmetic counter takes
   priority because $(( opened it at depth two, so its `))` unwinds fully
   before an enclosing $( sees its own closer. */
void	hj_depth_step(t_hjoin *h, int d)
{
	if (h->arith > 0)
		h->arith += d;
	else
		h->csub += d;
}

/* Terminate the joined buffer, release any unterminated here-doc tags, and
   hand the string to the caller (who owns it). */
char	*hj_finish(t_hjoin *h)
{
	size_t	i;

	i = 0;
	while (i < h->tags.len)
		xfree(((char **)h->tags.ctx)[i++]);
	xfree(h->tags.ctx);
	vec_ensure_space_n(&h->out, 1);
	((char *)h->out.ctx)[h->out.len] = '\0';
	return ((char *)h->out.ctx);
}
