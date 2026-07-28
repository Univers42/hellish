/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_range2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/15 01:59:37 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 01:59:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

void	exit_clean(t_shell *state, int code);

/* Decide whether to run the next command based on the operator that
   separates it from the previous one.  ; and newline always run.  &&
   short-circuits on failure; || short-circuits on success.  Ctrl-C
   (ctrl_c=true) stops execution regardless of operator so an interrupted
   pipeline does not stumble forward into the right-hand side. */
bool	should_execute(t_execution_state prev_status, t_tt prev_op)
{
	if (prev_status.ctrl_c)
		return (false);
	ft_assert(prev_status.status != -1);
	ft_assert(prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AND || prev_op == TT_OR || prev_op == TT_AMPERSAND);
	if (prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AMPERSAND)
		return (true);
	if (prev_op == TT_AND && prev_status.status == 0)
		return (true);
	if (prev_op == TT_OR && prev_status.status != 0)
		return (true);
	return (false);
}

/* set -e: exit when the AND-OR list's last *executed* command failed. This
   excludes the left operand of && / || (it isn't the one that ran last, e.g.
   `false && true` skips true so `false` is non-terminal), a negated pipeline
   (! ...), and any list run as an if/while/until condition (errexit_off). */
void	errexit_check(t_shell *state, t_execution_state st,
			bool ran, t_ast_node *last)
{
	if (state->errexit_off || !ran || st.status == 0)
		return ;
	if (last && last->node_type == AST_COMMAND_PIPELINE && last->negate)
		return ;
	if (state->should_exit || state->func_return || state->loop_break
		|| state->loop_continue || get_g_sig()->should_unwind)
		return ;
	fire_err_trap(state, st.status);
	if (state->opt_errexit)
		exit_clean(state, st.status);
}
