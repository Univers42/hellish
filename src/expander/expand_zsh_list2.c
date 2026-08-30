/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_list2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:15:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:15:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Whitespace split, for (z): runs collapse and leading/trailing space is
   dropped, the way a command line is split -- the opposite of (f), which
   keeps every empty field.  Both spellings exist in zsh precisely because
   the two answers differ, so neither may be implemented as the other. */
void	zl_split_ws(t_vec *l, const char *v)
{
	const char	*start;

	while (*v)
	{
		while (*v == ' ' || *v == '\t' || *v == '\n')
			v++;
		if (!*v)
			return ;
		start = v;
		while (*v && *v != ' ' && *v != '\t' && *v != '\n')
			v++;
		zl_push(l, ft_strndup(start, (size_t)(v - start)));
	}
}

/* Seed from an array or associative value, which arrives still encoded.
   (k) takes keys, (v) values, (kv) alternates them the way `${(kv)h}`
   flattens a hash into a name value name value list.
     On an INDEXED array (k) is a no-op and the values come back, which is
   not the guess -- indices would be the obvious answer, and are wrong.
   zsh 5.9 gives `x y z` for `${(k)arr}` on `arr=(x y z)`. */
void	zl_records(t_shell *state, t_zflags *f, t_vec *l, const char *val)
{
	t_assoc_it	it;
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;

	(void)state;
	if (assoc_is(val))
	{
		assoc_it_init(&it, val);
		while (assoc_next(&it))
		{
			if (zf_has(f, 'k'))
				zl_push(l, ft_strndup(it.k, it.kl));
			if (zf_has(f, 'v') || !zf_has(f, 'k'))
				zl_push(l, ft_strndup(it.v, it.vl));
		}
		return ;
	}
	cur = val + 1;
	while (arr_next(&cur, &idx, &v, &vl))
		zl_push(l, ft_strndup(v, (size_t)vl));
}

/* Order two elements: (n) compares the leading integers rather than the
   bytes, so v2 sorts before v10 -- which is the only reason (n) exists and
   the only case where a byte sort gives an answer that looks right and is
   not.  Falls back to a byte compare when neither side starts with a digit,
   so a mixed list still has a total order instead of an arbitrary one. */
static int	zl_cmp(const char *a, const char *b, bool numeric)
{
	long	x;
	long	y;

	if (numeric && (ft_isdigit((unsigned char)*a)
			|| ft_isdigit((unsigned char)*b)))
	{
		x = ft_atol(a);
		y = ft_atol(b);
		if (x != y)
			return ((x > y) - (x < y));
	}
	return (ft_strcmp((char *)a, (char *)b));
}

/* Insertion sort, then (u) de-duplicates in place.  Insertion sort because
   these lists are branch names and $path entries -- tens of elements, not
   thousands -- and it is the shortest correct thing that is also stable, so
   (u) after (o) keeps the first of each equal run like zsh does.
     Both are skipped outright unless the expansion is an array: `"${(o)a}"`
   in double quotes joins first and the sort has nothing to sort.  That is
   zsh, verified against 5.9, and it is not what the spelling suggests. */
void	zl_order(t_zflags *f, t_vec *l)
{
	char	**a;
	size_t	i;
	size_t	j;
	int		dir;

	if (!f->array)
		return ;
	if (!zf_has(f, 'o') && !zf_has(f, 'O'))
		return (zl_uniq(f, l));
	dir = 1;
	if (zf_has(f, 'O'))
		dir = -1;
	a = (char **)l->ctx;
	i = 1;
	while (i < l->len)
	{
		j = i++;
		while (j > 0 && zl_cmp(a[j - 1], a[j], zf_has(f, 'n')) * dir > 0)
		{
			zl_swap(&a[j - 1], &a[j]);
			j--;
		}
	}
	zl_uniq(f, l);
}
