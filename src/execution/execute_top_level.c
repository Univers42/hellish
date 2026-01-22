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


void	execute_top_level(t_shell *state)
{
	t_executable_node	exe;
	t_exe_res			res;

	/* Prepare execution node */
	exe = (t_executable_node){.infd = 0, .outfd = 1, .node = &state->tree,
		.modify_parent_context = true};
	/* initialize redirs vector for this execution node */
	vec_init(&exe.redirs);
	exe.redirs.elem_size = sizeof(int);
	state->heredoc_idx = 0;

	/* gather heredocs and execute the tree */
	if (!get_g_sig()->should_unwind)
		gather_heredocs(state, &state->tree, false);
	if (!get_g_sig()->should_unwind)
		res = execute_tree_node(state, &exe);
	else
		res = res_status(CANCELED);

	/* Clean up process substitutions */
	cleanup_proc_subs(state);

	if (res.c_c)
	{
		if (state->input_method == INP_READLINE)
			ft_eprintf("\n");
		else
			state->should_exit = true;
	}
	state->last_cmd_status_res = res;
}