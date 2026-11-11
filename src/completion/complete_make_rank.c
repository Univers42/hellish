/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_rank.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "complete_make.h"
#include <string.h>

/* make <TAB> ordering: the targets the history shows being used come
** first, best frecency first (history_frecency.c, the ranking the inline
** suggestion uses); the rest follow alphabetically.
**
** An entry counts for a target when its FIRST word is a command the
** provider table maps to comp_make (make, gmake, any path to them) and
** the target is one of its whitespace-separated words, once per entry.
**
** Caveat: this reads words, not make's command line. `make -C re test`
** credits a target named `re` for the directory, and in `make a && make b`
** the `b` counts because the entry starts with make: a target can rank
** higher than it deserves, but is never hidden. `sudo make x` is not
** counted, and a word with `;`, a quote or a backslash glued on never
** equals a target name, so those under-credit. HELLISH_SUGGEST_STRATEGY=
** recent, or an alphabetical list, is the way out. */

/* Alphabetical, the way readline sorts when it is allowed to (strcoll),
   so an unused-only list looks exactly as it always did. */
int	mk_cmp_name(const void *x, const void *y)
{
	const t_mk_use	*a;
	const t_mk_use	*b;

	a = x;
	b = y;
	return (strcoll(a->name, b->name));
}

/* Used targets first: best score, then newest; ties, and every unused
   target (score 0), by name. */
int	mk_cmp_use(const void *x, const void *y)
{
	const t_mk_use	*a;
	const t_mk_use	*b;

	a = x;
	b = y;
	if (a->score != b->score && a->score < b->score)
		return (1);
	if (a->score != b->score)
		return (-1);
	if (a->newest != b->newest && a->newest < b->newest)
		return (1);
	if (a->newest != b->newest)
		return (-1);
	return (mk_cmp_name(x, y));
}

/* Is the entry a make invocation: its first word names the make provider
   (the same table row that decides `make <TAB>` itself). */
bool	mk_is_make_entry(const char *e)
{
	size_t	n;
	size_t	b;

	while (*e == ' ' || *e == '\t')
		e++;
	n = 0;
	while (e[n] && e[n] != ' ' && e[n] != '\t')
		n++;
	b = prov_base(e, n);
	return (prov_is_make(e + b, n - b));
}

/* Binary search of the name-sorted `u` for the word w[0..n); NULL if it
   is not a target. */
static t_mk_use	*mk_find(t_mk_use *u, size_t cnt, const char *w, size_t n)
{
	size_t	lo;
	size_t	hi;
	size_t	mid;
	int		c;

	lo = 0;
	hi = cnt;
	while (lo < hi)
	{
		mid = lo + (hi - lo) / 2;
		c = ft_strncmp(u[mid].name, w, n);
		if (!c && u[mid].name[n])
			c = 1;
		if (!c)
			return (&u[mid]);
		if (c < 0)
			lo = mid + 1;
		else
			hi = mid;
	}
	return (NULL);
}

/* Credit entry `idx` (text `e`) to every distinct target among its words.
   `seen` stops a word repeated in one entry from counting twice. */
void	mk_count_entry(t_mk_rank *r, const char *e, size_t idx)
{
	t_mk_use	*t;
	size_t		n;

	while (*e)
	{
		while (*e == ' ' || *e == '\t')
			e++;
		n = 0;
		while (e[n] && e[n] != ' ' && e[n] != '\t')
			n++;
		t = mk_find(r->u, r->cnt, e, n);
		if (t && n && t->seen != idx + 1)
		{
			t->seen = idx + 1;
			t->score = hist_score_add(t->score, r->len, idx, r->recent);
			t->newest = idx;
		}
		e += n;
	}
}
