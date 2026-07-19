/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_getopts2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Advance the character position within the current word (for multi-option
   words like -abc). When we exhaust the word, move to the next word and
   reset the char position to the first option character (1, past the '-'). */
static void	gopt_advance(t_shell *state, t_getopts *g, char *cur, int pos)
{
	if (!cur[pos + 1])
	{
		g->optind++;
		state->getopts_pos = 0;
	}
	else
		state->getopts_pos = pos + 1;
}

/* Handle an option letter that is not in optstring. In silent mode, stuff
   the bad letter into OPTARG; otherwise print the standard error message
   and unset OPTARG (POSIX). Either way, set the name variable to '?' and
   advance past the letter. */
static int	one_option_bad(t_shell *state, t_getopts *g, char *cur, int pos)
{
	if (g->silent)
		gopt_set_char(state, "OPTARG", cur[pos]);
	else
	{
		ft_eprintf("%s: illegal option -- %c\n", state->dft_ctx, cur[pos]);
		try_unset(state, "OPTARG");
	}
	gopt_set_name(state, g, '?');
	gopt_advance(state, g, cur, pos);
	return (gopt_commit_optind(state, g->optind), 0);
}

/* Process one recognised option letter from `cur[pos]`. If the option takes
   an argument (next char in optstring is ':'), consume it (attached or from
   the next word) via gopt_want_arg — when that fails the error path already
   stored ':' or '?' in the name variable, so it must NOT be overwritten
   with the option letter here. Then commit OPTIND. Returns 0 when more
   options remain, non-zero when the outer loop should stop. */
int	one_option(t_shell *state, t_vec argv, t_getopts *g, char *cur)
{
	int		pos;
	char	*spec;

	pos = state->getopts_pos;
	spec = ft_strchr(g->optstring, cur[pos]);
	if (!spec || cur[pos] == ':')
		return (one_option_bad(state, g, cur, pos));
	if (spec[1] == ':')
	{
		if (gopt_want_arg(state, argv, g, cur))
			gopt_set_name(state, g, cur[pos]);
		return (gopt_commit_optind(state, g->optind), 0);
	}
	try_unset(state, "OPTARG");
	gopt_set_name(state, g, cur[pos]);
	gopt_advance(state, g, cur, pos);
	return (gopt_commit_optind(state, g->optind), 0);
}

/* Read $OPTIND and reconcile it with the private scan state, mirroring
   bash. If the stored value is not the exact string gopt_commit_optind
   last wrote (pointer identity, see shell.h), the user assigned OPTIND
   and the intra-word position restarts. Clamp to at least 1 (a script
   may set it to 0 or below) and to at most count+1: when the argument
   list shrank under a stale index, bash lands one past the last word so
   the next scan reports end-of-options there (spec: OPTIND=1 after a
   loop over `set --`, not the stale 4). */
static void	gopt_read_optind(t_shell *state, t_getopts *g)
{
	t_env	*e;

	g->optind = 1;
	e = env_get(&state->env, "OPTIND");
	if (!e || !e->value || e->value != state->getopts_ref)
		state->getopts_pos = 0;
	if (e && e->value)
		g->optind = ft_atoi(e->value);
	if (g->optind < 1)
		g->optind = 1;
	if (g->optind > g->count + 1)
	{
		g->optind = g->count + 1;
		state->getopts_pos = 0;
	}
}

/* Initialise the getopts working state from the call's arguments. The
   option count is the number of extra args passed on the command line,
   or $# if none were given; it must be known before gopt_read_optind
   can clamp a stale index. An invalid name variable does not abort
   parsing: bash still consumes the option and updates OPTARG/OPTIND,
   only the name assignment is suppressed and the call reports failure. */
void	gopt_init(t_shell *state, t_vec argv, t_getopts *g)
{
	char	*cnt;

	g->optstring = ((char **)argv.ctx)[1];
	g->name = ((char **)argv.ctx)[2];
	g->silent = (g->optstring[0] == ':');
	g->bad_name = !ft_is_valid_ident(g->name);
	if (argv.len > 3)
		g->count = (int)argv.len - 3;
	else
	{
		cnt = env_expand(state, "#");
		if (cnt)
			g->count = ft_atoi(cnt);
		else
			g->count = 0;
	}
	gopt_read_optind(state, g);
}
