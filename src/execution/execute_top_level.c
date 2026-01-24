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

	exe = create_exe_node(0, 1, &state->tree, true);
	vec_init(&exe.redirs);
	exe.redirs.elem_size = sizeof(int);
	state->heredoc_idx = 0;
	if (!get_g_sig()->should_unwind)
		gather_heredocs(state, &state->tree, false);
	if (!get_g_sig()->should_unwind)
		res = execute_tree_node(state, &exe);
	else
		res = res_status(CANCELED);
	cleanup_proc_subs(state);
	if (res.c_c)
	{
		if (state->input_method == INP_READLINE)
			ft_eprintf("\n");
		else
			state->should_exit = true;
	}
	state->last_cmd_status_res = res;
	verbose(CLAP_PRINT, "");
}
