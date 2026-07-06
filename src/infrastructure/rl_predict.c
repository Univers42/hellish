/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_predict.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "rl_ghost_ai.h"
#include <readline/history.h>

#define PRED_SLOTS 8

/* Count `s` into the first matching (or first free) tally slot. */
static void	tally_add(const char **lines, int *counts, const char *s)
{
	int	i;

	i = 0;
	while (i < PRED_SLOTS)
	{
		if (!lines[i])
			return (lines[i] = s, counts[i] = 1, (void)0);
		if (!ft_strcmp(lines[i], s))
			return ((void)counts[i]++);
		i++;
	}
}

/* The winning successor: highest count, at least 2 sightings, short enough to
   ghost, SINGLE-LINE (a raw \n in a ghost desyncs the cursor), and a command
   that actually resolves (never predict a typo). */
static const char	*tally_best(const char **lines, int *counts)
{
	int	i;
	int	best;

	best = -1;
	i = 0;
	while (i < PRED_SLOTS)
	{
		if (lines[i] && counts[i] >= 2
			&& (best < 0 || counts[i] > counts[best]))
			best = i;
		i++;
	}
	if (best < 0 || ft_strlen(lines[best]) > 256
		|| ft_strchr(lines[best], '\n') || !cmd_resolvable(lines[best]))
		return (NULL);
	return (lines[best]);
}

/* What people most commonly run next IN GENERAL, when personal history has no
   strong signal: a small curated table of shell idioms, matched on the first
   word(s) of the command that just ran (word boundary enforced). Never suggests
   the command itself again, and only commands that resolve on this system.
   ponytail: static table; extend pairs freely as habits emerge. */
static const char	*idiom_next(const char *last)
{
	static const char	*tab[] = {"git add", "git commit", "git commit",
		"git push", "git status", "git add -A", "git diff", "git add -A",
		"git pull", "git status", "git clone", "ls", "make", "make test",
		"mkdir", "ls", "cd", "ls", "unzip", "ls", "tar", "ls", NULL};
	size_t				l;
	int					i;

	i = 0;
	while (tab[i])
	{
		l = ft_strlen(tab[i]);
		if (!ft_strncmp(last, tab[i], l)
			&& (last[l] == '\0' || last[l] == ' ')
			&& ft_strcmp(last, tab[i + 1]) != 0
			&& cmd_resolvable(tab[i + 1]))
			return (tab[i + 1]);
		i += 2;
	}
	return (NULL);
}

/* Most frequent historical successor of the command that just ran (= the
   newest history entry): what usually follows `git add .` in YOUR history is
   what you'll likely type next. One O(history) pass, in-memory only. */
static const char	*predict_next(void)
{
	HIST_ENTRY	**h;
	const char	*lines[PRED_SLOTS];
	int			counts[PRED_SLOTS];
	const char	*last;
	int			i;

	h = history_list();
	if (!h || history_length < 3 || !h[history_length - 1])
		return (NULL);
	ft_bzero(lines, sizeof(lines));
	ft_bzero(counts, sizeof(counts));
	last = h[history_length - 1]->line;
	i = 0;
	while (i < history_length - 1)
	{
		if (h[i] && h[i + 1] && !ft_strcmp(h[i]->line, last))
			tally_add(lines, counts, h[i + 1]->line);
		i++;
	}
	return (tally_best(lines, counts));
}

/* The empty-prompt prediction: personal habit first (most frequent successor
   in YOUR history), then the general-usage idiom table. Computed once per
   readline child (history is frozen for the child's life, so the memo is sound
   and every redisplay after the first costs nothing). Borrowed; do not free. */
const char	*ghost_predict_empty(void)
{
	static const char	*cached;
	static int			done;
	HIST_ENTRY			**h;

	if (done)
		return (cached);
	done = 1;
	cached = predict_next();
	h = history_list();
	if (!cached && h && history_length > 0 && h[history_length - 1])
		cached = idiom_next(h[history_length - 1]->line);
	return (cached);
}
