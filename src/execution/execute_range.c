/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_range.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 16:26:46 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 19:28:13 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

static void	execute_then(t_executable_node *exe,
							t_ast_node *child,
							t_shell *state,
							t_exe_res *status)
{
	t_executable_node	exe_curr;

	exe_curr = *exe;
	exe_curr.node = child;
	*status = execute_tree_node(state, &exe_curr);
	if (status->pid != -1)
		exe_res_set_status(status);
	ft_assert(status->status != -1);
}

/* Execute a range of chi{ldren [start, end)
as a sequence in the current process.
   This handles && and || chaining within the range. */
t_exe_res	execute_range(t_shell *state, t_executable_node *exe,
							size_t start, size_t end)
{
	t_exe_res			status;
	t_tt				op;
	size_t				i;
	t_ast_node			*child;

	status = res_status(0);
	op = TT_SEMICOLON;
	i = start;
	while (i < end)
	{
		child = &((t_ast_node *)exe->node->children.ctx)[i];
		if (child->node_type == AST_TOKEN)
		{
			op = child->token.tt;
			i++;
			continue ;
		}
		if (should_execute(status, op))
			execute_then(exe, child, state, &status);
		i++;
	}
	return (status);
}

/* Execute a command sequence in the background */
t_exe_res	execute_range_background(t_shell *state, t_executable_node *exe,
										size_t start, size_t end)
{
	pid_t		pid;
	int			null_fd;
	t_exe_res	res;

	pid = fork();
	if (pid == 0)
	{
		setpgid(0, 0);
		null_fd = open("/dev/null", O_RDONLY);
		if (null_fd >= 0)
			(dup2(null_fd, STDIN_FILENO), close(null_fd));
		(signal(SIGINT, SIG_IGN), signal(SIGQUIT, SIG_IGN));
		(signal(SIGTSTP, SIG_IGN), signal(SIGTTIN, SIG_IGN));
		signal(SIGTTOU, SIG_IGN);
		res = execute_range(state, exe, start, end);
		exit(res.status);
	}
	if (pid < 0)
		critical_error_errno_context("fork");
	procsub_close_fds_parent(state);
	state->bg_job_count++;
	ft_printf("[%d] %d\n", state->bg_job_count, pid);
	return (res_status(0));
}
