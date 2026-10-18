/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_nsearch.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/18 03:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/18 03:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* The byte (or EOF / READERR) the reader is about to hand readline, after
** freeing the search string's undo list when that byte ends a
** non-incremental search (Alt-N / Alt-P, stock) without searching.
**
** The search reads its string into the line buffer with an undo list of
** its own: it saves the line, sets rl_undo_list to NULL and inserts. On
** Enter it frees that list before searching; on abort it puts the saved
** line back, overwriting rl_undo_list, and readline 8.1 (Ubuntu 22.04's)
** loses the list -- a leak per aborted search, reported by ASan at exit.
** Freeing it here, before readline sees the key, leaves nothing to lose,
** and a readline that frees it itself finds NULL.
**
** Caveat: the abort keys are readline 8.1's (search.c,
** _rl_nsearch_dispatch): ^G, ^C, Rubout or ^H on an empty string, and a
** failed read. A key a later readline adds to that list leaks again; a
** key it drops loses only the undo history of the search string. */
int	rl_nsearch_reclaim(int c)
{
	if (!RL_ISSTATE(RL_STATE_NSEARCH))
		return (c);
	if (c == '\a' || c == 3 || c < 0
		|| ((c == '\b' || c == 127) && rl_point == 0))
		rl_free_undo_list();
	return (c);
}
