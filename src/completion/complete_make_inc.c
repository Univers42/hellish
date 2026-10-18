/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_inc.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Directives and includes of a Makefile line.  See complete_make.c. */

#include "complete_make.h"

/* Read one included file by name (literal names only) and scan it as
   level 1, so what it includes is never followed. */
static void	mk_inc_one(t_mk_scan *m, const char *name)
{
	t_mk_scan	sub;
	char		*buf;
	size_t		len;
	size_t		i;

	i = 0;
	while (name[i])
	{
		if (ft_strchr("$*?[]%~", name[i++]))
			return ;
	}
	buf = mk_slurp(name, &len);
	if (!buf)
		return ;
	sub = *m;
	sub.level = 1;
	sub.def = 0;
	mk_scan(&sub, buf, len);
	xfree(buf);
}

/* include FILES...: each name in turn, only from the Makefile itself. */
static void	mk_include(t_mk_scan *m, char *l)
{
	char	*w;

	while (m->level == 0 && *l)
	{
		while (mk_blank(*l))
			l++;
		w = l;
		while (*l && !mk_blank(*l))
			l++;
		if (*l)
			*l++ = '\0';
		if (*w)
			mk_inc_one(m, w);
	}
}

static int	mk_in_list(const char *l, const char **list)
{
	int	i;

	i = -1;
	while (list[++i])
	{
		if (mk_word_is(l, list[i]))
			return (i);
	}
	return (-1);
}

/* 1 when the line was a directive (and is consumed). */
int	mk_directive(t_mk_scan *m, char *l)
{
	static const char	*inc[] = {"include", "-include", "sinclude", NULL};
	static const char	*dir[] = {"ifeq", "ifneq", "ifdef", "ifndef",
		"else", "endif", "export", "unexport", "override", "vpath", NULL};
	int					i;

	i = mk_in_list(l, inc);
	if (i >= 0)
	{
		mk_include(m, l + ft_strlen(inc[i]));
		return (1);
	}
	return (mk_in_list(l, dir) >= 0);
}
