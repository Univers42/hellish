/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_cache2.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Refilling the Makefile cache.  See complete_make_cache.c. */

#include "complete_make.h"

/* Reread `name` into the cache; the old entry goes first so a file that
   became unreadable leaves nothing stale behind. */
static t_vec	*mk_fill(t_make_cache *c, const char *name, struct stat *sb)
{
	char		*buf;
	size_t		len;
	t_mk_scan	m;

	buf = mk_slurp(name, &len);
	mk_clear(c);
	if (!buf)
		return (NULL);
	c->name = ft_strdup(name);
	mk_stamp(c, sb);
	c->targets.elem_size = sizeof(char *);
	m.out = &c->targets;
	m.level = 0;
	m.def = 0;
	mk_scan(&m, buf, len);
	xfree(buf);
	return (&c->targets);
}

/* The targets of the cwd's Makefile, or NULL when there is none. */
t_vec	*mk_load(t_shell *st)
{
	struct stat	sb;
	const char	*name;

	if (!mk_pick(&sb, &name))
		return (NULL);
	if (mk_same(&st->make_cache, name, &sb))
		return (&st->make_cache.targets);
	return (mk_fill(&st->make_cache, name, &sb));
}
