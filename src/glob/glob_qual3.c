/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_qual3.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 11:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 11:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include "ft_glob.h"

/* The first index of the trailing run of literal-ish tokens, and the byte
   the run ends at.
     The tokeniser emits one token per character, so `[*](DNY2)` is an
   asterisk followed by SIX literals -- reading only the last one saw a `)`
   and gave up. G_SLASH is in the run too: the `/` of `(N/)` is a path
   separator to the tokeniser and a qualifier letter here, so a run that
   stopped at it left the group unparsed and the pattern literal.
     ADJACENCY IS CHECKED, not assumed. Tokens are usually contiguous slices
   of one buffer, but a word assembled from EXPANDED pieces -- `a=$?` becomes
   literals from different allocations -- is not, and treating it as one span
   computed a length across unrelated memory. gq_from_text then scanned
   backwards off the front of a buffer looking for a `(` and segfaulted, on
   input with no parenthesis in it at all.
     The walk now stops at the first gap, so a non-contiguous run degrades to
   the last token alone rather than to a wild pointer. */
size_t	gq_run_start(t_vec_glob *g, const char **end)
{
	t_glob	*a;
	size_t	i;

	a = (t_glob *)g->ctx;
	*end = a[g->len - 1].start + a[g->len - 1].len;
	i = g->len - 1;
	while (i > 0 && (a[i - 1].ty == G_LITERAL || a[i - 1].ty == G_SLASH)
		&& a[i - 1].start + a[i - 1].len == a[i].start)
		i--;
	return (i);
}

/* Drop or trim every token that falls inside the qualifier group, so the
   walk globs the pattern without it. */
void	gq_trim(t_vec_glob *g, size_t from, const char *open)
{
	t_glob	*a;

	a = (t_glob *)g->ctx;
	while (g->len > from && a[g->len - 1].start >= open)
		g->len--;
	if (g->len > from && a[g->len - 1].start + a[g->len - 1].len > open)
		a[g->len - 1].len = (int)(open - a[g->len - 1].start);
	if (g->len > from && a[g->len - 1].len == 0)
		g->len--;
}
