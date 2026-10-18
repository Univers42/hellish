/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_suggest8.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include <stdlib.h>

/* libc qsort, not ft_qsort: ft_qsort pivots on the last element, so a run
** of identical entries -- already in ranking order -- costs O(n^2), 6.8 s
** for one key over 20000 copies (autosuggest_test F13). libc is
** O(n log n) on glibc, musl and macOS, and both comparators here are
** total orders, so every libc gives the same ranking. */

/* The ranking behind the ghost and Alt-N / Alt-P: the distinct texts of
** the candidates (rl_suggest_cands), best first. No hash table: libft's
** hash dups every key, which is one malloc per candidate per key. Sorting
** the reused scratch by text puts equal texts side by side, so one walk
** scores each group; a second sort orders the groups. */

/* Descending order for a comparator: the larger sorts first. */
static int	sg_three(uint64_t x, uint64_t y)
{
	if (x == y)
		return (0);
	if (x < y)
		return (1);
	return (-1);
}

/* Text first, and within one text the newest occurrence first, so a group
   starts with the entry that will stand for it. */
static int	sg_cmp_text(const void *x, const void *y)
{
	const t_sg_ent	*a;
	const t_sg_ent	*b;
	int				c;

	a = x;
	b = y;
	c = ft_strcmp(a->text, b->text);
	if (c)
		return (c);
	return (sg_three(a->idx, b->idx));
}

/* Best score first; on a tie the newest. The representatives are distinct
   entries, so this is a strict total order and the sort needs no
   stability. */
static int	sg_cmp_rank(const void *x, const void *y)
{
	const t_sg_ent	*a;
	const t_sg_ent	*b;

	a = x;
	b = y;
	if (a->score != b->score)
		return (sg_three(a->score, b->score));
	return (sg_three(a->idx, b->idx));
}

/* Collapse each run of equal texts to its first entry (the newest) holding
   the run's score, in place: the write cursor never passes the read one.
   Returns the number of distinct texts. */
static size_t	sg_fold(t_sg_ent *s, size_t n, size_t len, bool recent)
{
	size_t	r;
	size_t	w;

	r = 0;
	w = 0;
	while (r < n)
	{
		s[w] = s[r];
		s[w].score = 0;
		while (r < n && !ft_strcmp(s[r].text, s[w].text))
			s[w].score = hist_score_add(s[w].score, len, s[r++].idx,
					recent);
		w++;
	}
	return (w);
}

/* Rank the history for the line: sg_rank gets the history index of each
** distinct candidate text, best first (score, then newest). The newest
** occurrence stands for its text. Returns how many.
**
** `recent` is today's order exactly: newest first, repeats collapsed.
** Only indices are kept: the scratch holds pointers into the history
** that a later `history -c` would leave dangling.
**
** Cost: O(m log m) for m candidates; no allocation once the two vectors
** have grown to the window. */
size_t	rl_suggest_rank(t_shell *state, t_rl *l)
{
	t_sg_ent	*s;
	size_t		n;
	size_t		i;

	rl_suggest_cyc_clear(l);
	l->sg_rank.elem_size = sizeof(size_t);
	n = rl_suggest_cands(state, l);
	s = (t_sg_ent *)l->sg_scr.ctx;
	qsort(s, n, sizeof(*s), sg_cmp_text);
	n = sg_fold(s, n, state->hist.hist_cmds.len, l->sg_recent);
	qsort(s, n, sizeof(*s), sg_cmp_rank);
	i = 0;
	while (i < n)
		vec_push(&l->sg_rank, &s[i++].idx);
	return (l->sg_rank.len);
}
