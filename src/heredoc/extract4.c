/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   extract4.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Detect whether accumulated input ends in the MIDDLE of a heredoc body -- a
   `<<word` operator was seen whose delimiter line has not arrived yet. Used to
   keep gathering lines so a heredoc on a simple command nested inside a compound
   command accumulates its whole body before the parser runs. */

#include "heredoc_private.h"

/* Advance *p over one heredoc body; true if its delimiter line was reached
   before end-of-input, false if the input ran out first (still pending). */
static bool	body_terminated(const char **p, t_hd *s)
{
	const char	*ls;

	while (**p)
	{
		ls = *p;
		while (**p && **p != '\n')
			(*p)++;
		if (**p == '\n')
			(*p)++;
		if (is_delim_line(ls, *p - ls, s))
			return (true);
	}
	return (false);
}

/* Free the t_hd vec built while scanning (each spec owns its delim string). */
static void	free_hd_vec(t_vec *v)
{
	size_t	i;

	i = 0;
	while (i < v->len)
		xfree(((t_hd *)v->ctx)[i++].delim);
	xfree(v->ctx);
}

/* Every heredoc operator found on the current command line (the last k specs)
   must have its delimiter present in the remaining input. Returns false on the
   first one that does not -- meaning the input is still mid-heredoc. */
static bool	line_bodies_ok(const char **p, t_vec *v, int k)
{
	int	base;
	int	i;

	base = (int)v->len - k;
	i = 0;
	while (i < k)
	{
		if (!**p || !body_terminated(p, &((t_hd *)v->ctx)[base + i]))
			return (false);
		i++;
	}
	return (true);
}

/* True when `str` ends mid-heredoc. Walks line by line like collect_specs,
   tokenising each command line to find `<<word` operators, then skipping each
   body to its delimiter; the first body that runs off the end means more input
   is needed. */
bool	heredoc_incomplete(const char *str)
{
	t_vec		v;
	const char	*p;
	const char	*ls;
	int			k;

	if (!str || !ft_strnstr(str, "<<", ft_strlen(str)))
		return (false);
	vec_init(&v);
	v.elem_size = sizeof(t_hd);
	p = str;
	while (*p)
	{
		ls = p;
		while (*p && *p != '\n')
			p++;
		k = specs_on_line(ls, p - ls + (*p == '\n'), v.len, &v);
		p += (*p == '\n');
		if (k > 0 && !line_bodies_ok(&p, &v, k))
			return (free_hd_vec(&v), true);
	}
	return (free_hd_vec(&v), false);
}
