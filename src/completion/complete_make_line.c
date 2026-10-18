/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_line.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* What a logical Makefile line is: a recipe, a define block, a directive,
   an include, or (the default) a candidate rule.  See complete_make.c. */

#include "complete_make.h"

int	mk_blank(char c)
{
	return (c == ' ' || c == '\t');
}

/* `w` is the first word of `l`: ended by a blank, '(' (ifeq(...)) or NUL. */
int	mk_word_is(const char *l, const char *w)
{
	size_t	n;

	n = ft_strlen(w);
	return (!ft_strncmp(l, w, n)
		&& (!l[n] || mk_blank(l[n]) || l[n] == '('));
}

/* Cut a '#' comment (a backslash escapes it), then trim both ends. */
static char	*mk_clean(char *l)
{
	size_t	i;

	i = 0;
	while (l[i] && !(l[i] == '#' && (i == 0 || l[i - 1] != '\\')))
		i++;
	l[i] = '\0';
	while (i > 0 && mk_blank(l[i - 1]))
		l[--i] = '\0';
	while (mk_blank(*l))
		l++;
	return (l);
}

/* define NAME ... endef is skipped whole (they nest).  `override define`
   and `export define` open one as well. */
static int	mk_define(t_mk_scan *m, char *l)
{
	if (m->def)
	{
		if (mk_word_is(l, "endef"))
			m->def--;
		else if (mk_word_is(l, "define"))
			m->def++;
		return (1);
	}
	while (mk_word_is(l, "override") || mk_word_is(l, "export"))
	{
		while (*l && !mk_blank(*l))
			l++;
		while (mk_blank(*l))
			l++;
	}
	if (!mk_word_is(l, "define"))
		return (0);
	m->def = 1;
	return (1);
}

void	mk_line(t_mk_scan *m, char *l)
{
	if (!m->def && l[0] == '\t')
		return ;
	l = mk_clean(l);
	if (mk_define(m, l) || mk_directive(m, l))
		return ;
	mk_rule(m, l);
}
