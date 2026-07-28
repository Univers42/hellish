/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_scan3.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"

/* << at the cursor, first < already emitted: copy the second <, queue a
   here-doc delimiter capture (2 = <<- tab-stripping form), and pull in a
   clustered - or a third < (<<< here-string turns back into a plain
   redirection target inside asc_op_extra). */
static void	asc_op_hd(t_ascan *a, t_ascan_src *s)
{
	vec_push(&a->out, &s->s[s->pos]);
	s->pos++;
	a->pend_hd = 1;
	if (s->s[s->pos] == '-')
		a->pend_hd = 2;
	if (s->s[s->pos] == '-' || s->s[s->pos] == '<')
		asc_op_extra(a, s);
}

/* Operator characters at the cursor: copy them through and update the
   position state.  ; | & ( ) and friends put the scanner back in command
   position; < and > (plus clustered <<, >>, <&, >&, >|, <>) arm the
   redirection-target rule instead, and << additionally queues a here-doc
   delimiter capture via asc_op_hd. */
void	asc_op(t_ascan *a)
{
	t_ascan_src	*s;
	char		c;

	s = &a->src[a->depth - 1];
	c = s->s[s->pos];
	vec_push(&a->out, &c);
	s->pos++;
	a->wstart = true;
	if (c == ';' || c == '|' || c == '&' || c == '(' || c == ')')
	{
		a->cmd_pos = true;
		a->after_redir = false;
		return ;
	}
	if (c == '<' && s->s[s->pos] == '<')
	{
		asc_op_hd(a, s);
		return ;
	}
	a->after_redir = true;
	if (ft_strchr("<>&|", s->s[s->pos]))
		asc_op_extra(a, s);
}

/* Copy one extra clustered operator character (the - of <<-, the second
   > of >>, the & of >&...).  A third < turns <<< (here-string) back into
   a plain redirection target. */
void	asc_op_extra(t_ascan *a, t_ascan_src *s)
{
	char	c;

	c = s->s[s->pos];
	vec_push(&a->out, &c);
	s->pos++;
	if (c == '<' && a->pend_hd)
	{
		a->pend_hd = 0;
		a->after_redir = true;
	}
}

/* Register a here-doc delimiter captured right after <<.  One level of
   quoting is stripped for the match (POSIX: the delimiter is the word
   after quote removal); the original spelling is still emitted by the
   caller so the real here-doc machinery sees it untouched. */
void	asc_hd_delim(t_ascan *a, const char *w, size_t len)
{
	char	*d;
	size_t	i;
	size_t	j;

	a->wstart = false;
	if (a->pend_hd && a->hd_n < ALIAS_HD_MAX)
	{
		d = xmalloc(len + 1);
		i = 0;
		j = 0;
		while (i < len)
		{
			if (w[i] != '\'' && w[i] != '"' && w[i] != '\\')
			{
				d[j] = w[i];
				j++;
			}
			i++;
		}
		d[j] = '\0';
		a->hd_delim[a->hd_n] = d;
		a->hd_strip[a->hd_n] = (a->pend_hd == 2);
		a->hd_n++;
	}
	a->pend_hd = 0;
}

/* Copy one here-doc body line through verbatim; when it equals the
   pending delimiter (after optional leading-tab stripping for <<-), that
   here-doc is finished and the queue advances. */
void	asc_hd_body(t_ascan *a, t_ascan_src *s)
{
	size_t		start;
	size_t		cmp;
	size_t		len;

	start = s->pos;
	while (s->s[s->pos] && s->s[s->pos] != '\n')
		s->pos++;
	cmp = start;
	if (a->hd_strip[0])
	{
		while (s->s[cmp] == '\t')
			cmp++;
	}
	len = s->pos - cmp;
	if (s->s[s->pos] == '\n')
		s->pos++;
	vec_push_nstr(&a->out, s->s + start, s->pos - start);
	if (len == ft_strlen(a->hd_delim[0])
		&& !ft_strncmp(s->s + cmp, a->hd_delim[0], len))
		asc_hd_drop(a);
}
