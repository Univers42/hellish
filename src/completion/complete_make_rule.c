/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_rule.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* The rule half of the Makefile scan: find the colon, split the target
   list, keep the names worth offering.  See complete_make.c. */

#include "complete_make.h"

/* `$(...)` and `${...}` starting at l[i]: the index after it. */
static size_t	mk_skip_ref(const char *l, size_t i)
{
	char	o;
	char	c;
	int		d;

	o = l[i + 1];
	if (o != '(' && o != '{')
		return (i + 1);
	c = ')';
	if (o == '{')
		c = '}';
	d = 0;
	i++;
	while (l[i])
	{
		if (l[i] == o)
			d++;
		else if (l[i] == c && --d == 0)
			return (i + 1);
		i++;
	}
	return (i);
}

/* The first ':' is a rule's unless it opens := ::= :::= (an assignment). */
static long	mk_colon_kind(const char *l, size_t i)
{
	if (l[i + 1] == '=')
		return (-1);
	if (l[i + 1] == ':' && (l[i + 2] == '='
			|| (l[i + 2] == ':' && l[i + 3] == '=')))
		return (-1);
	return ((long)i);
}

/* Index of the rule colon, or -1 when the line is not a rule: an '=' before
   the first colon makes it an assignment (V = a:b, V ?= c:d, V += e:f). */
static long	mk_colon(const char *l)
{
	size_t	i;

	i = 0;
	while (l[i])
	{
		if (l[i] == '$')
		{
			i = mk_skip_ref(l, i);
			continue ;
		}
		if (l[i] == '=')
			return (-1);
		if (l[i] == ':')
			return (mk_colon_kind(l, i));
		i++;
	}
	return (-1);
}

/* l is "targets : ..." with the colon found: take every target. */
void	mk_rule(t_mk_scan *m, char *l)
{
	long		c;
	size_t		n;

	c = mk_colon(l);
	if (c < 0)
		return ;
	l[c] = '\0';
	while (*l)
	{
		while (mk_blank(*l))
			l++;
		n = 0;
		while (l[n] && !mk_blank(l[n]))
			n++;
		mk_add(m->out, l, n);
		l += n;
	}
}
