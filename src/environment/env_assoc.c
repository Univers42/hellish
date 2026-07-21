/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_assoc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* Read-side helpers for associative arrays. Records are "key<US>value"
   joined by RS, prefixed with ARR_ASSOC_MAGIC; keys are arbitrary strings
   (kept in insertion order, no numeric semantics). */

static char	*assoc_join(const char *val, char sep, int want_keys);

bool	assoc_is(const char *val)
{
	return (val != NULL && val[0] == ARR_ASSOC_MAGIC);
}

/* Record iterator: *cur starts at val+1. Yields key slice and value
   slice; false at end. */
bool	assoc_next(const char **cur, const char **k, int *kl,
			const char **v, int *vl)
{
	const char	*p;

	p = *cur;
	if (!p || !*p)
		return (false);
	*k = p;
	*kl = 0;
	while (p[*kl] && p[*kl] != ARR_US)
		(*kl)++;
	p += *kl;
	if (*p == ARR_US)
		p++;
	*v = p;
	*vl = 0;
	while (p[*vl] && p[*vl] != ARR_RS)
		(*vl)++;
	p += *vl;
	if (*p == ARR_RS)
		p++;
	*cur = p;
	return (true);
}

int	assoc_count(const char *val)
{
	const char	*cur;
	const char	*k;
	const char	*v;
	int			kl;
	int			vl;
	int			n;

	if (!assoc_is(val))
		return (0);
	cur = val + 1;
	n = 0;
	while (assoc_next(&cur, &k, &kl, &v, &vl))
		n++;
	return (n);
}

/* Heap copy of the value stored under `key` (klen bytes), or NULL. */
char	*assoc_get(const char *val, const char *key, int klen)
{
	const char	*cur;
	const char	*k;
	const char	*v;
	int			kl;
	int			vl;

	if (!assoc_is(val))
		return (NULL);
	cur = val + 1;
	while (assoc_next(&cur, &k, &kl, &v, &vl))
	{
		if (kl == klen && ft_strncmp(k, key, klen) == 0)
			return (ft_strndup(v, vl));
	}
	return (NULL);
}

/* Space-joined keys (${!h[@]}) or values (${h[@]}, sep=' '). */
static char	*assoc_join(const char *val, char sep, int want_keys)
{
	t_string	out;
	const char	*cur;
	const char	*k;
	const char	*v;
	int			kl;
	int			vl;

	vec_init(&out);
	out.elem_size = 1;
	cur = "";
	if (assoc_is(val))
		cur = val + 1;
	while (assoc_next(&cur, &k, &kl, &v, &vl))
	{
		if (out.len && sep)
			vec_push_char(&out, sep);
		if (want_keys)
			vec_push_nstr(&out, (char *)k, kl);
		else
			vec_push_nstr(&out, (char *)v, vl);
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

char	*assoc_keys(const char *val)
{
	return (assoc_join(val, ' ', 1));
}

char	*assoc_values(const char *val, char sep)
{
	return (assoc_join(val, sep, 0));
}
