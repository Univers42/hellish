/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   res_utils2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

void	exit_clean(t_shell *state, int code);

/* Is this line over? A ^C unwind or an interactive expansion error
   (state->discard_line) both mean: run nothing more of it. One question,
   asked by the word expander, the simple-command executor, the list
   loops and the streamed-input loop, so they cannot disagree. */
bool	exec_aborting(t_shell *state)
{
	return (get_g_sig()->should_unwind || state->discard_line);
}

/* The status of a simple command whose expansion failed: a ^C unwind is
   130, a bad substitution or a ${u:?} that fired is 1 -- bash's status
   for the line it discards -- and an ambiguous redirect keeps its own; a
   redirect error a non-interactive shell must not survive exits here. */
t_execution_state	res_expand_failed(t_shell *state, bool fatal)
{
	if (get_g_sig()->should_unwind)
		return (res_status(CANCELED));
	if (state->discard_line)
		return (res_status(1));
	if (fatal)
		exit_clean(state, 1);
	return (res_status(AMBIGUOUS_REDIRECT));
}
