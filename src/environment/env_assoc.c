/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_assoc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* Read-side helpers for associative arrays. Records are "key<US>value"
   joined by RS, prefixed with ARR_ASSOC_MAGIC; keys are arbitrary strings
   (kept in insertion order, no numeric semantics). */

bool	assoc_is(const char *val)
{
	return (val != NULL && val[0] == ARR_ASSOC_MAGIC);
}

/* Prime an iterator on any value: a non-assoc value yields an empty
   iteration rather than a crash, so callers need no pre-check. */
void	assoc_it_init(t_assoc_it *it, const char *val)
{
	it->cur = "";
	if (assoc_is(val))
		it->cur = val + 1;
}

/* Record iterator: fills the key slice (k/kl) and value slice (v/vl);
   false at end. */
bool	assoc_next(t_assoc_it *it)
{
	const char	*p;

	p = it->cur;
	if (!p || !*p)
		return (false);
	it->k = p;
	it->kl = 0;
	while (p[it->kl] && p[it->kl] != ARR_US)
		it->kl++;
	p += it->kl;
	if (*p == ARR_US)
		p++;
	it->v = p;
	it->vl = 0;
	while (p[it->vl] && p[it->vl] != ARR_RS)
		it->vl++;
	p += it->vl;
	if (*p == ARR_RS)
		p++;
	it->cur = p;
	return (true);
}

int	assoc_count(const char *val)
{
	t_assoc_it	it;
	int			n;

	assoc_it_init(&it, val);
	n = 0;
	while (assoc_next(&it))
		n++;
	return (n);
}

/* Heap copy of the value stored under `key` (klen bytes), or NULL. */
char	*assoc_get(const char *val, const char *key, int klen)
{
	t_assoc_it	it;

	assoc_it_init(&it, val);
	while (assoc_next(&it))
	{
		if (it.kl == klen && ft_strncmp(it.k, key, klen) == 0)
			return (ft_strndup(it.v, it.vl));
	}
	return (NULL);
}
