/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_always.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 22:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 22:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* `{ body } always { cleanup }` -- zsh's finally.
**
** The cleanup block runs whatever the body did, and "whatever" is the whole
** point: a plugin uses this to put the line editor's buffer back after a
** case statement that may have returned early.  So the three ways out of the
** body all have to be caught, and each needs its own save:
**
**   a failing command   -- status, which the always block may overwrite
**   `return`            -- func_return, or the cleanup never runs
**   `break` / `continue`-- loop_ctl, same
**
** They are restored AFTER the cleanup runs, so a `return` inside the body
** still returns once the cleanup has finished, and a cleanup block that
** itself returns wins -- which is zsh's rule and the one that makes the
** construct usable for unwinding.
**
** The exit STATUS is the BODY's, always -- the cleanup block's own status is
** discarded.  Checked against zsh 5.9, because the plausible rule is the
** other one: `{ echo a } always { false }` looks like it should be 1 and is
** 0, and `{ false } always { true }` looks like 0 and is 1.  A cleanup block
** exists to tidy up, and letting its status through would mean every plugin
** that ends one with a test silently rewrote the result of the body.
*/

/* Save the three ways out, and clear them so the cleanup block runs on a
   shell that is not already unwinding. */
static void	always_park(t_shell *state, int *save)
{
	save[0] = state->func_return;
	save[1] = state->loop_break;
	save[2] = state->loop_continue;
	state->func_return = 0;
	state->loop_break = 0;
	state->loop_continue = 0;
}

/* Put them back, unless the cleanup block raised its own. */
static void	always_unpark(t_shell *state, int *save)
{
	if (!state->func_return)
		state->func_return = save[0];
	if (!state->loop_break)
		state->loop_break = save[1];
	if (!state->loop_continue)
		state->loop_continue = save[2];
}

t_execution_state	execute_brace_group(t_shell *state,
						t_executable_node *exe)
{
	t_executable_node	sub;
	t_execution_state	body;
	t_ast_node			*group;
	int					save[3];

	group = exe->node;
	sub = *exe;
	sub.node = vec_idx(&group->children, 0);
	body = execute_tree_node(state, &sub);
	if (group->children.len < 2)
		return (body);
	always_park(state, save);
	sub = *exe;
	sub.node = vec_idx(&group->children, 1);
	execute_tree_node(state, &sub);
	always_unpark(state, save);
	set_cmd_status(state, body);
	return (body);
}
