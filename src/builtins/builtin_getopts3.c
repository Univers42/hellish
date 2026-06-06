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

/* Signal end-of-options: set the name var to '?' and advance OPTIND past
   the terminator (one extra for "--" so it is not presented as an operand).
   Returns 1 so the calling while loop exits. */
static int	gopt_done(t_shell *state, t_getopts *g, char *cur)
{
	gopt_set_char(state, g->name, '?');
	state->getopts_pos = 0;
	if (cur && cur[1] == '-')
		gopt_commit_optind(state, g->optind + 1);
	else
		gopt_commit_optind(state, g->optind);
	return (1);
}

/* getopts optstring name [arg ...]: the POSIX option-parsing loop helper.
   Each call sets `name` to the next option letter and $OPTARG when the
   option takes an argument. Returns 0 while options remain, 1 when done or
   on error (so `while getopts …; do` works naturally).

   The state->getopts_pos field persists the character position within a
   multi-option word (e.g. `-abc`) between calls — reset to 0 means "start
   at the beginning of the next word". The tail-recursive call when pos is
   exhausted advances to the next word cleanly without a separate loop. */
int	builtin_getopts(t_shell *state, t_vec argv)
{
	t_getopts	g;
	char		*cur;

	if (argv.len < 3)
		return (ft_eprintf(
				"getopts: usage: getopts optstring name [arg]\n"), 2);
	gopt_init(state, argv, &g);
	if (state->getopts_pos < 1)
		state->getopts_pos = 1;
	if (g.optind <= g.count)
		cur = gopt_arg(state, argv, g.optind);
	else
		cur = NULL;
	if (!cur || cur[0] != '-' || !cur[1]
		|| (cur[1] == '-' && !cur[2]))
		return (gopt_done(state, &g, cur));
	if (!cur[state->getopts_pos])
	{
		state->getopts_pos = 0;
		gopt_commit_optind(state, g.optind + 1);
		return (builtin_getopts(state, argv));
	}
	return (one_option(state, argv, &g, cur));
}
