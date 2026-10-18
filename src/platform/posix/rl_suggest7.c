/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_suggest7.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* Cycling: Alt-N and Alt-P. */

/* Put the entry at history index `idx` on screen as the ghost. */
static void	sg_show(t_shell *state, size_t idx)
{
	t_rl	*l;
	char	**a;

	l = &state->rl;
	a = (char **)state->hist.hist_cmds.ctx;
	xfree(l->sg_ent);
	l->sg_ent = ft_strdup(a[idx]);
	l->sg_txt = l->sg_ent + rl_end;
}

/* Start a cycle on the ghost on screen: the ranking is made once, and
   its first entry is that ghost (the history has not changed, see
   rl_suggest_cyc_sync). */
static bool	sg_cyc_begin(t_shell *state, t_rl *l)
{
	if (!rl_suggest_rank(state, l))
		return (false);
	l->sg_cyc_k = 0;
	l->sg_cyc_line = ft_strdup(rl_line_buffer);
	return (true);
}

/* Alt-N: the next lower-ranked distinct match. At the last one there is
   nothing more: bell, and the ghost stays (no wrap). */
int	rl_suggest_older(int count, int key)
{
	t_shell	*state;
	t_rl	*l;

	state = rl_suggest_ghost_state();
	if (!state)
		return (rl_noninc_forward_search(count, key));
	l = &state->rl;
	rl_suggest_cyc_sync(state, l);
	if (!l->sg_cyc_line && !sg_cyc_begin(state, l))
		return (rl_suggest_bell());
	if (l->sg_cyc_k + 1 >= l->sg_rank.len)
		return (rl_suggest_bell());
	l->sg_cyc_k++;
	sg_show(state, ((size_t *)l->sg_rank.ctx)[l->sg_cyc_k]);
	return (0);
}

/* Alt-P: back to the better-ranked alternative shown before. At the best
   match there is nothing better: bell. */
int	rl_suggest_newer(int count, int key)
{
	t_shell	*state;
	t_rl	*l;

	state = rl_suggest_ghost_state();
	if (!state)
		return (rl_noninc_reverse_search(count, key));
	l = &state->rl;
	rl_suggest_cyc_sync(state, l);
	if (!l->sg_cyc_line || l->sg_cyc_k == 0)
		return (rl_suggest_bell());
	l->sg_cyc_k--;
	sg_show(state, ((size_t *)l->sg_rank.ctx)[l->sg_cyc_k]);
	return (0);
}
