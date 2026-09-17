/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_subst4.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Where ${v/pat/rep} splits pat from rep -- bash's rule, bounded.
**
** The split is the first '/' that is not quoted, not escaped and not
** inside a nested expansion. It used to be the first '/' of any kind:
**
**     x=a/b; echo ${x//"/"/_}        bash: a_b     hellish: segfault
**     x=a/b; echo ${x/\//_}          bash: a_b     hellish: a/b
**     x=a/b; echo ${x//$(echo /)/_}  bash: a_b     hellish: a/b
**
** The first line cut the pattern to a lone `"`, which the reparser then
** read past the end of. The text is a slice of the word, not a C string:
** span_skip never reads past it, and a span left open means "no split". */
static int	subst_pat_end(const char *s, int i, int len)
{
	while (i >= 0 && i < len && s[i] != '/')
	{
		if (s[i] == '\\')
			i += 2;
		else
			i = span_skip(s, i, len);
	}
	if (i < 0 || i > len)
		return (len);
	return (i);
}

/* The pattern of a ${v/pat/rep} body is [*start, return). `g` counts the
   global slash, `a` is subst_anchor's answer.
     One quirk is bash's, kept on purpose: when the pattern itself begins
   with '/', that slash is pattern, not separator. ${x///} deletes every
   slash and ${x////r} replaces them with r. The anchor, when present, is
   the first byte instead, so the rule never applies to /# and /%. */
int	subst_span(t_trim_ctx ctx, int g, int a, int *start)
{
	int	from;

	*start = ctx.name_len + 1 + g;
	from = *start;
	if (!a && from < ctx.slen && ctx.name[from] == '/')
		from++;
	*start += (a != 0);
	return (subst_pat_end(ctx.name, from, ctx.slen));
}
