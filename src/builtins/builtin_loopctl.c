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

/* break [n]: exit from the n innermost enclosing loops (default 1). */
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

/* continue [n]: resume the n-th enclosing loop (default 1). */
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
