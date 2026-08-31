/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_pretty2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "ft_glob.h"

/* The reporting and preset half of `pretty` (see builtin_pretty.c). */

/* Modes are BUNDLES of the feature bits, nothing more -- there is no
   "current mode" stored anywhere, because storing one would let it drift
   out of step with the individual toggles underneath it. `pretty mode X`
   is a one-shot assignment; `pretty -p` afterwards reports the features,
   which is the truth. */
static unsigned int	pretty_mode_bits(const char *name, bool *found)
{
	t_pret			*t;
	unsigned int	all;

	*found = true;
	if (!ft_strcmp(name, "plain"))
		return (0);
	if (!ft_strcmp(name, "friendly"))
		return (SHOPT_LITHIST | SHOPT_CDSPELL | SHOPT_CHECKWINSIZE);
	if (!ft_strcmp(name, "full"))
	{
		all = 0;
		t = pretty_table();
		while (t->name)
			all |= t++->bit;
		return (all);
	}
	return (*found = false, 0);
}

/* pretty mode NAME -- replace the whole feature set with the preset's.
   Assignment, not merge: `pretty mode plain` has to be able to turn things
   OFF, or there is no way back to bash-identical behaviour. */
int	pretty_mode(t_shell *state, t_vec argv, int first)
{
	unsigned int	bits;
	unsigned int	owned;
	t_pret			*t;
	bool			found;

	if (first >= (int)argv.len)
		return (ft_eprintf("%s: pretty: mode: expected plain, friendly or"
				" full\n", state->ctx), 2);
	bits = pretty_mode_bits(((char **)argv.ctx)[first], &found);
	if (!found)
		return (ft_eprintf("%s: pretty: mode: %s: unknown mode (try"
				" `pretty list`)\n", state->ctx,
				((char **)argv.ctx)[first]), 2);
	owned = 0;
	t = pretty_table();
	while (t->name)
		owned |= t++->bit;
	state->shopt = (state->shopt & ~owned) | bits;
	return (glob_opts_sync(state), 0);
}

/* pretty [-p] -- what is on. Plain form is for reading; -p emits the exact
   lines to paste into ~/.hellishrc, which is what makes a configuration
   portable between machines instead of something you have to remember. */
int	pretty_show(t_shell *state, bool reusable)
{
	t_pret	*t;
	int		on;

	on = 0;
	t = pretty_table();
	while (t->name)
	{
		if (state->shopt & t->bit)
		{
			on++;
			if (reusable)
				ft_printf("pretty on %s\n", t->name);
			else
				ft_printf("  \033[32mon \033[0m %-18s %s\n", t->name,
					t->desc);
		}
		t++;
	}
	if (!on && reusable)
		ft_printf("pretty mode plain\n");
	else if (!on)
		ft_printf("  nothing on (bash-identical). Try `pretty list`.\n");
	return (0);
}

/* pretty list -- every feature with its state, then the presets. This is
   the discoverability the whole builtin exists for: one command that says
   what can be changed and what each thing does. */
int	pretty_list(t_shell *state)
{
	t_pret		*t;
	const char	*state_word;

	ft_printf("features (pretty on|off NAME):\n");
	t = pretty_table();
	while (t->name)
	{
		state_word = "off";
		if (state->shopt & t->bit)
			state_word = "on";
		ft_printf("  %-3s %-18s %s\n", state_word, t->name, t->desc);
		t++;
	}
	ft_printf("\nmodes (pretty mode NAME):\n");
	ft_printf("  %-10s %s\n", "plain",
		"everything off -- bash-identical behaviour");
	ft_printf("  %-10s %s\n", "friendly",
		"multiline-history, cd-spell, resize-aware");
	ft_printf("  %-10s %s\n", "full", "every feature above");
	ft_printf("\n`pretty -p` prints your current set as ~/.hellishrc"
		" lines.\n");
	return (0);
}

/* The glob-option mirror this file used to keep half a copy of now lives in
   builtin_shopt.c as glob_opts_sync -- one list, three writers. Its comment
   records what the half-copy cost. */
