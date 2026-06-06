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
   the bad letter into OPTARG; otherwise print the standard error message.
   Either way, set the name variable to '?' and advance past the letter. */
static int	one_option_bad(t_shell *state, t_getopts *g, char *cur, int pos)
{
	if (g->silent)
		gopt_set_char(state, "OPTARG", cur[pos]);
	else
		ft_eprintf("%s: illegal option -- %c\n", state->dft_ctx, cur[pos]);
	gopt_set_char(state, g->name, '?');
	gopt_advance(state, g, cur, pos);
	return (gopt_commit_optind(state, g->optind), 0);
}

/* Process one recognised option letter from `cur[pos]`. If the option takes
   an argument (next char in optstring is ':'), consume it (attached or from
   the next word) via gopt_want_arg. Then set the name variable and commit
   OPTIND. Returns 0 when more options remain, non-zero when the outer loop
   should stop. */
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
		gopt_want_arg(state, argv, g, cur);
		gopt_set_char(state, g->name, cur[pos]);
		return (gopt_commit_optind(state, g->optind), 0);
	}
	gopt_set_char(state, g->name, cur[pos]);
	gopt_advance(state, g, cur, pos);
	return (gopt_commit_optind(state, g->optind), 0);
}

/* Initialise the getopts working state from the call's arguments and the
   current value of $OPTIND. The option count is the number of extra args
   passed on the command line, or $# if none were given. Clamp OPTIND to
   at least 1 to guard against a script that accidentally sets it to 0. */
void	gopt_init(t_shell *state, t_vec argv, t_getopts *g)
{
	char	*oi;
	char	*cnt;

	g->optstring = ((char **)argv.ctx)[1];
	g->name = ((char **)argv.ctx)[2];
	g->silent = (g->optstring[0] == ':');
	oi = env_expand(state, "OPTIND");
	if (oi)
		g->optind = ft_atoi(oi);
	else
		g->optind = 1;
	if (g->optind < 1)
		g->optind = 1;
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
}
