/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_hist_prefix.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "rl_private.h"
#include "env.h"
#include "zle.h"
#include <strings.h>

/* HELLISH_HISTORY_PREFIX, read live from the shell's own variables so an
   assignment at the prompt, in the rc file or inherited all count. Off
   only for 0 / off (any case); unset, empty or anything else is on. */
static int	hp_wanted(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, "HELLISH_HISTORY_PREFIX");
	if (!e || !e->value)
		return (1);
	if (!ft_strcmp(e->value, "0") || !strcasecmp(e->value, "off"))
		return (0);
	return (1);
}

/* Interactive reads only, right before rl_preinit: note what is wanted and
** switch the two hooks below on. Doing it inside rl_preinit's caller keeps
** `bind -p` (which also runs rl_preinit, in -c mode) exactly as bash's.
** Up / Down become history search by the text before the cursor, or plain
** previous / next with HELLISH_HISTORY_PREFIX=0|off. */
void	rl_hist_prefix_sync(t_shell *state)
{
	state->rl.hist_live = true;
	state->rl.hist_want = hp_wanted(state);
}

/* rl_preinit, after the editing mode is set and BEFORE the user's `bind`
** / `bindkey` lines are replayed: park the slots on a marker (only when
** something changed -- the wanted state or new requests -- so a quiet
** prompt costs a few compares). A mode switch only selects another keymap,
** it rebuilds none, so the arrows survive it; its replay is "pending"
** (binds_done rewound), which is what triggers the work. */
void	rl_hist_prefix_pre(t_rl *l)
{
	int	bit;

	l->hist_open = false;
	if (!l->hist_live)
		return ;
	l->hist_open = l->hist_want != l->hist_on
		|| l->binds_done < zle_binds()->len
		|| l->bind_lines_done < bind_lines()->len;
	bit = 0;
	while (l->hist_open && bit < 8)
		rl_hist_arm(l, bit++);
}

/* rl_preinit, after the replay: settle each parked slot. */
void	rl_hist_prefix_post(t_rl *l)
{
	int	bit;

	if (!l->hist_open)
		return ;
	bit = 0;
	while (bit < 8)
		rl_hist_settle(l, bit++);
	l->hist_on = l->hist_want;
	l->hist_open = false;
}
