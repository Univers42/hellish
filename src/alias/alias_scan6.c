/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_scan6.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"

/* True while the given name is an active expansion source: POSIX forbids
   re-expanding a name whose own value is still being spliced, which is
   what turns `alias e='echo e'; e` into `echo e` instead of a loop. */
bool	asc_active(t_ascan *a, const char *w, size_t len)
{
	int	i;

	i = 1;
	while (i < a->depth)
	{
		if (a->src[i].name && ft_strlen(a->src[i].name) == len
			&& !ft_strncmp(a->src[i].name, w, len))
			return (true);
		i++;
	}
	return (false);
}

/* A newline inside a spliced alias value would break the driver's
   one-statement-list-per-cycle model (everything after the first
   statement would be dropped), so it is rewritten exactly like the
   cmdhist joiner rewrites history: "; " where a separator is valid, a
   plain space where the grammar still expects a command (right after
   do/then/{/;/&&...) — which is precisely what cmd_pos tracks. */
void	asc_emit_sep(t_ascan *a)
{
	if (!a->cmd_pos)
		vec_push_nstr(&a->out, ";", 1);
	vec_push_nstr(&a->out, " ", 1);
}

/* A newline ends the command: emit it, then, if here-doc delimiters are
   queued, copy the body lines through verbatim (aliases never expand
   inside a here-doc body).  Inside an alias value the newline becomes a
   "; " (or " ") instead — see asc_emit_sep.  The next word is in
   command position. */
void	asc_newline(t_ascan *a)
{
	t_ascan_src	*s;
	char		c;

	s = &a->src[a->depth - 1];
	c = '\n';
	s->pos++;
	if (a->depth > 1 && a->hd_n == 0)
		asc_emit_sep(a);
	else
		vec_push(&a->out, &c);
	while (a->hd_n > 0 && s->s[s->pos])
		asc_hd_body(a, s);
	a->cmd_pos = (a->cs_n == 0 || a->cs_st == CS_BODY);
	a->chk_next = false;
	a->after_redir = false;
	a->pend_hd = 0;
	a->wstart = true;
}
