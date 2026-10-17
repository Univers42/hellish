/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_suggest6.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* Partial accept (Alt-F, Ctrl-Right) and cycling (Alt-N, Alt-P) of the
   inline suggestion. Each falls back to the key's stock readline function
   when there is no history ghost to act on. */

/* The shell state when a history ghost is on screen and can be acted on:
   a ghost a widget set as POSTDISPLAY is not an entry of the history, so
   it is neither cycled nor taken a word at a time. */
t_shell	*rl_suggest_ghost_state(void)
{
	t_shell	**cell;

	cell = zle_state_cell();
	if (!*cell || (*cell)->rl.sg_post || !rl_suggest_live(&(*cell)->rl))
		return (NULL);
	return (*cell);
}

/* Ring the bell for a key with nowhere to go; the key itself succeeds. */
int	rl_suggest_bell(void)
{
	rl_ding();
	return (0);
}

/* Alt-F / Ctrl-Right: take the next word of the ghost into the line. A
** word is the blanks (space, tab) at the front of the ghost and the run of
** non-blank bytes after them; nothing else separates words, so punctuation
** stays inside one (`-m`, `a/b`, `x=1`). UTF-8 needs no care: no multibyte
** sequence holds 0x20 or 0x09. The rest stays the ghost: the cached entry
** still matches the longer line, so rl_suggest_find returns it at once. */
int	rl_suggest_word(int count, int key)
{
	t_shell	*state;
	char	*g;
	char	*w;
	size_t	n;

	state = rl_suggest_ghost_state();
	if (!state)
		return (rl_forward_word(count, key));
	g = rl_suggest_live(&state->rl);
	n = 0;
	while (g[n] == ' ' || g[n] == '\t')
		n++;
	while (g[n] && g[n] != ' ' && g[n] != '\t')
		n++;
	w = ft_strndup(g, n);
	rl_suggest_cyc_clear(&state->rl);
	rl_insert_text(w);
	xfree(w);
	return (0);
}
