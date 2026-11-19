/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_slot_env.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "env.h"
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

/* HELLISH_AUTOSUGGEST, read the same way but the other way round: off
   unless it is 1 or on (any case). A terminal that cannot move the cursor
   (TERM unset or dumb) never gets a ghost. */
static int	as_wanted(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, "TERM");
	if (!e || !e->value || !ft_strcmp(e->value, "dumb"))
		return (0);
	e = env_get(&state->env, "HELLISH_AUTOSUGGEST");
	if (!e || !e->value)
		return (0);
	return (!ft_strcmp(e->value, "1") || !strcasecmp(e->value, "on"));
}

/* HELLISH_SUGGEST_MAX: how many of the newest history entries a search
   looks at. Anything that is not a plain number >= 1 is the default; more
   than nine digits is "all of it" for any history there can be. */
static int	sg_limit(t_shell *state)
{
	t_env	*e;
	size_t	i;

	e = env_get(&state->env, "HELLISH_SUGGEST_MAX");
	if (!e || !e->value || !e->value[0])
		return (1000);
	i = 0;
	while (e->value[i] && ft_isdigit(e->value[i]))
		i++;
	if (e->value[i] || i > 9 || ft_atoi(e->value) < 1)
		return (1000);
	return (ft_atoi(e->value));
}

/* HELLISH_SUGGEST_STRATEGY: how suggestions are ranked. Exactly `recent`
   is newest first; unset, empty or anything else (typos included) is the
   default, frecency, so a misspelt value never silently changes order to
   the old one. */
static bool	sg_wants_recent(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, "HELLISH_SUGGEST_STRATEGY");
	return (e && e->value && !ft_strcmp(e->value, "recent"));
}

/* Interactive reads only, right before rl_preinit: note what is wanted and
** switch the slot hooks on. Doing it inside rl_preinit's caller keeps
** `bind -p` (which also runs rl_preinit, in -c mode) exactly as bash's.
** Up / Down become history search by the text before the cursor (off with
** HELLISH_HISTORY_PREFIX=0|off); Right / End / ^F accept the suggestion
** while HELLISH_AUTOSUGGEST is on. */
void	rl_slot_sync(t_shell *state)
{
	state->rl.slot_live = true;
	state->rl.feat_want[FEAT_HIST] = hp_wanted(state);
	state->rl.feat_want[FEAT_ACCEPT] = as_wanted(state);
	state->rl.sg_max = sg_limit(state);
	state->rl.sg_recent = sg_wants_recent(state);
}
