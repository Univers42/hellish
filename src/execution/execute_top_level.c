/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:41 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:41 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Entry point called once per parsed statement by the REPL.  It wraps the
   whole execution lifecycle for one top-level tree: pre-scan heredocs
   (gather_heredocs fills temp files before any child forks), execute the
   tree, clean up process-substitution fds, and record the exit status.
   Ctrl-C while running a script (non-interactive: metinp != INP_RL) sets
   should_exit so the script aborts rather than just printing a newline.
   The verbose(CLAP_PRINT) call at the end is the "execution applause"
   hook -- a hook for debug/test output after each top-level command. */
void	execute_top_level(t_shell *state)
{
	t_executable_node	exe;
	t_execution_state	res;

	exe = create_exe_node(0, 1, &state->tree, true);
	vec_init(&exe.redirs);
	exe.redirs.elem_size = sizeof(int);
	state->heredoc_idx = 0;
	state->hd_defer = state->cycle_has_hd
		&& (state->tree.node_type == AST_SIMPLE_LIST
			|| state->tree.node_type == AST_COMPOUND_LIST);
	if (!get_g_sig()->should_unwind && !state->hd_defer
		&& state->cycle_has_hd)
		gather_heredocs(state, &state->tree, false);
	if (!get_g_sig()->should_unwind)
		res = execute_tree_node(state, &exe);
	else
		res = res_status(CANCELED);
	cleanup_proc_subs(state);
	if (res.ctrl_c)
	{
		if (state->metinp == INP_RL)
			ft_eprintf("\n");
		else
			state->should_exit = true;
	}
	state->last_cmd_st_exe = res;
	verbose(CLAP_PRINT, "");
}
