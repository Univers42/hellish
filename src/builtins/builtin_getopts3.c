/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_getopts3.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Every write to the user's option variable funnels through here: when
   the variable name is invalid (e.g. `getopts a opt-`) bash still parses
   the option and updates OPTARG/OPTIND, but never assigns the bad name —
   builtin_getopts then reports failure once, after the scan. */
void	gopt_set_name(t_shell *state, t_getopts *g, char c)
{
	if (g->bad_name)
		return ;
	gopt_set_char(state, g->name, c);
}

/* Signal end-of-options: set the name var to '?', unset OPTARG (bash
   drops it on every end-of-options flavor; dash keeps the stale value)
   and advance OPTIND past the terminator when the current word is
   exactly "--" (so it is not presented as an operand). An operand that
   merely contains a dash (like "x-") must NOT trigger that extra
   advance. Returns 1 so the calling while loop exits. */
static int	gopt_done(t_shell *state, t_getopts *g, char *cur)
{
	gopt_set_name(state, g, '?');
	try_unset(state, "OPTARG");
	state->getopts_pos = 0;
	if (cur && cur[0] == '-' && cur[1] == '-' && !cur[2])
		gopt_commit_optind(state, g->optind + 1);
	else
		gopt_commit_optind(state, g->optind);
	return (1);
}

/* One scan step. The intra-word position (state->getopts_pos) survives
   between calls so multi-option words like `-abc` resume mid-word; a
   position past the end of the current word (the argument list changed
   underneath a mid-word scan) advances to the next word and retries via
   the tail call. */
static int	gopt_run(t_shell *state, t_vec argv, t_getopts *g)
{
	char	*cur;

	if (state->getopts_pos < 1)
		state->getopts_pos = 1;
	if (g->optind <= g->count)
		cur = gopt_arg(state, argv, g->optind);
	else
		cur = NULL;
	if (!cur || cur[0] != '-' || !cur[1]
		|| (cur[1] == '-' && !cur[2]))
		return (gopt_done(state, g, cur));
	if (!cur[state->getopts_pos])
	{
		state->getopts_pos = 1;
		g->optind++;
		gopt_commit_optind(state, g->optind);
		return (gopt_run(state, argv, g));
	}
	return (one_option(state, argv, g, cur));
}

/* getopts optstring name [arg ...]: the POSIX option-parsing loop helper.
   Each call sets `name` to the next option letter and $OPTARG when the
   option takes an argument. Returns 0 while options remain, 1 when done
   (so `while getopts …; do` works naturally). An invalid `name` returns
   1 after the scan — matching bash --posix, which parses first and only
   fails when binding the variable (dash returns 2 here). */
int	builtin_getopts(t_shell *state, t_vec argv)
{
	t_getopts	g;
	int			ret;

	if (argv.len < 3)
		return (ft_eprintf(
				"getopts: usage: getopts optstring name [arg]\n"), 2);
	gopt_init(state, argv, &g);
	ret = gopt_run(state, argv, &g);
	if (g.bad_name)
		return (ft_eprintf("%s: getopts: `%s': not a valid identifier\n",
				state->dft_ctx, g.name), 1);
	return (ret);
}
