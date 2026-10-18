/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_cache.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* The one-entry Makefile cache: pick the file, decide whether the parsed
   targets are still good, refill them when not.  See complete_make.c for
   the rules and the include limitation. */

#include "complete_make.h"

/* GNU make's default order; the first REGULAR file wins (stat follows
   symlinks), a directory of that name is skipped like a missing file. */
int	mk_pick(struct stat *sb, const char **name)
{
	static const char	*cand[] = {"GNUmakefile", "makefile", "Makefile",
		NULL};
	int					i;

	i = -1;
	while (cand[++i])
	{
		if (stat(cand[i], sb) == 0 && S_ISREG(sb->st_mode))
		{
			*name = cand[i];
			return (1);
		}
	}
	return (0);
}

void	mk_clear(t_make_cache *c)
{
	size_t	i;

	i = 0;
	while (i < c->targets.len)
		xfree(((char **)c->targets.ctx)[i++]);
	xfree(c->targets.ctx);
	xfree(c->order.ctx);
	xfree(c->name);
	ft_bzero(c, sizeof(*c));
}

/* Teardown, from free_all_state. */
void	mk_cache_free(t_shell *st)
{
	mk_clear(&st->make_cache);
}

int	mk_same(t_make_cache *c, const char *name, struct stat *sb)
{
	return (c->name && !ft_strcmp(c->name, name)
		&& c->dev == (long long)sb->st_dev
		&& c->ino == (long long)sb->st_ino
		&& c->size == (long long)sb->st_size
		&& c->mtime_s == (long long)sb->ST_MTIM.tv_sec
		&& c->mtime_ns == (long long)sb->ST_MTIM.tv_nsec);
}

void	mk_stamp(t_make_cache *c, struct stat *sb)
{
	c->dev = (long long)sb->st_dev;
	c->ino = (long long)sb->st_ino;
	c->size = (long long)sb->st_size;
	c->mtime_s = (long long)sb->ST_MTIM.tv_sec;
	c->mtime_ns = (long long)sb->ST_MTIM.tv_nsec;
}
