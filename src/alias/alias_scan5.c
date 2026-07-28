/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_scan5.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"

/* Blanks are copied through; they leave both cmd_pos and the armed
   chk_next untouched (spaces may separate an alias from the word its
   trailing blank makes eligible). */
void	asc_scan_blank(t_ascan *a, t_ascan_src *s, char c)
{
	vec_push(&a->out, &c);
	s->pos++;
	a->wstart = true;
}

/* A comment runs to the end of the line; the newline itself is left for
   asc_newline so here-doc queue handling stays in one place. */
void	asc_comment(t_ascan *a)
{
	t_ascan_src	*s;

	s = &a->src[a->depth - 1];
	while (s->s[s->pos] && s->s[s->pos] != '\n')
	{
		vec_push(&a->out, &s->s[s->pos]);
		s->pos++;
	}
}

/* Drop the front of the here-doc delimiter queue (matched, or discarded
   at end of scan for an unterminated body). */
void	asc_hd_drop(t_ascan *a)
{
	int	i;

	xfree(a->hd_delim[0]);
	i = 1;
	while (i < a->hd_n)
	{
		a->hd_delim[i - 1] = a->hd_delim[i];
		a->hd_strip[i - 1] = a->hd_strip[i];
		i++;
	}
	a->hd_n--;
}

/* Reserved words after which the parser still expects a command word, so
   alias expansion stays live for what follows. */
bool	asc_kw_cmdnext(const char *w, size_t len)
{
	static const char *const	kw[] = {"if", "then", "elif", "else",
		"while", "until", "do", "{", "!", NULL};
	int							i;

	i = 0;
	while (kw[i])
	{
		if (ft_strlen(kw[i]) == len && !ft_strncmp(kw[i], w, len))
			return (true);
		i++;
	}
	return (false);
}

/* Reserved words after which the next words are NOT commands (loop
   variables, case subjects, wordlists). */
bool	asc_kw_noncmd(const char *w, size_t len)
{
	static const char *const	kw[] = {"for", "case", "in", NULL};
	int							i;

	i = 0;
	while (kw[i])
	{
		if (ft_strlen(kw[i]) == len && !ft_strncmp(kw[i], w, len))
			return (true);
		i++;
	}
	return (false);
}
