/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_slot_run.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* rl_preinit, after the editing mode is set and BEFORE the user's `bind`
** / `bindkey` lines are replayed: park the slots of every feature that
** needs it on a marker (only when something changed -- the wanted state or
** new requests -- so a quiet prompt costs a few compares). A mode switch
** only selects another keymap, it rebuilds none, so the stock keys survive
** it; its replay is "pending" (binds_done rewound), which is what triggers
** the work. */
void	rl_slot_pre(t_rl *l)
{
	bool	pending;
	int		i;

	l->feat_open = 0;
	if (!l->slot_live)
		return ;
	pending = l->binds_done < zle_binds()->len
		|| l->bind_lines_done < bind_lines()->len;
	i = 0;
	while (i < RL_NFEAT)
	{
		if (pending || l->feat_want[i] != l->feat_on[i])
			l->feat_open |= 1u << i;
		i++;
	}
	i = 0;
	while (l->feat_open && i < RL_NSLOTS)
	{
		if (l->feat_open >> rl_slot_at(i).feat & 1)
			rl_slot_arm(l, i);
		i++;
	}
}

/* rl_preinit, after the replay: settle each parked slot. */
void	rl_slot_post(t_rl *l)
{
	int	i;

	i = 0;
	while (l->feat_open && i < RL_NSLOTS)
	{
		if (l->feat_open >> rl_slot_at(i).feat & 1)
			rl_slot_settle(l, i);
		i++;
	}
	i = 0;
	while (i < RL_NFEAT)
	{
		if (l->feat_open >> i & 1)
			l->feat_on[i] = l->feat_want[i];
		i++;
	}
	l->feat_open = 0;
}
