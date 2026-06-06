/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_loopctl.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* break and continue must be builtins because they affect the CURRENT shell's
   loop state. A forked child setting state->loop_break would disappear.

   The loop_depth / loop_break / loop_continue fields are inspected after each
   iteration body returns: loop_break > 0 means "unwind one level and
   decrement", letting `break 2` bubble up through nested loops correctly.
   Out-of-range n is silently clamped to loop_depth (POSIX allows this). */

/* break [n]: set the break-out counter for the n innermost loops. */
int	builtin_break(t_shell *state, t_vec argv)
{
	int	n;

	n = 1;
	if (argv.len >= 2)
		n = ft_atoi(((char **)argv.ctx)[1]);
	if (n < 1)
		return (1);
	if (state->loop_depth == 0)
		return (0);
	if (n > state->loop_depth)
		n = state->loop_depth;
	state->loop_break = n;
	return (0);
}

/* continue [n]: skip to the next iteration of the n-th enclosing loop. Works
   like break but sets loop_continue instead, so the loop body knows to jump
   back to the condition test rather than exit the loop entirely. */
int	builtin_continue(t_shell *state, t_vec argv)
{
	int	n;

	n = 1;
	if (argv.len >= 2)
		n = ft_atoi(((char **)argv.ctx)[1]);
	if (n < 1)
		return (1);
	if (state->loop_depth == 0)
		return (0);
	if (n > state->loop_depth)
		n = state->loop_depth;
	state->loop_continue = n;
	return (0);
}
