/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_frecency.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"

/* Frecency: how much a history entry is worth as a suggestion, from how
** often AND how recently its text was run. Shared by the inline ghost
** and Alt-N/P (platform/posix/rl_suggest8.c) and by `make <TAB>`
** (completion/complete_make_rank.c), so both rank the same way.
**
** An occurrence's weight starts at 2^16 and halves every FRE_HALF newer
** entries, down to 1 once it is 16 halvings old; a text scores the sum of
** its occurrences. The score does not depend on what is being typed, so
** the best match for a prefix stays the best for every longer prefix it
** still matches, which is what lets the ghost skip rescanning per key.
**
** Caveat: age is a count of entries, not time. A burst of 64 unrelated
** commands halves a habit's weight, while a command last used a year ago
** in a quiet shell still weighs as much as one used yesterday; and
** FRE_HALF = 64 is a guess, not a measurement. HELLISH_SUGGEST_MAX bounds
** the window, so nothing older than it counts at all. Escape hatch:
** HELLISH_SUGGEST_STRATEGY=recent ignores frequency entirely. */
#define FRE_HALF 64
#define FRE_STEPS 16

/* Weight of an entry `age` entries older than the newest. */
uint32_t	hist_weight(size_t age)
{
	size_t	steps;

	steps = age / FRE_HALF;
	if (steps > FRE_STEPS)
		steps = FRE_STEPS;
	return ((uint32_t)(1u << FRE_STEPS) >> steps);
}

/* Fold the occurrence at index `idx` (of `len` entries) into `score`.
   `recent` makes the score the newest index (+1, so 0 stays "unused"),
   which orders newest first and collapses repeats. */
uint64_t	hist_score_add(uint64_t score, size_t len, size_t idx,
				bool recent)
{
	if (recent)
	{
		if ((uint64_t)idx + 1 > score)
			return ((uint64_t)idx + 1);
		return (score);
	}
	return (score + hist_weight(len - 1 - idx));
}

/* First index of the window of the newest `max` entries. */
size_t	hist_window_floor(size_t len, int max)
{
	if (max > 0 && len > (size_t)max)
		return (len - (size_t)max);
	return (0);
}
