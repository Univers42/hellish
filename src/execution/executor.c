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

#include "shell.h"
#include "redir.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include "../libft/libft.h"

#include <readline/readline.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "ft_printf.h"
# include "helpers.h"
# include "expander.h"
# include "input.h"
# include "executor_types.h"

// returns pid
t_exe_res	execute_subshell(t_shell *state, t_executable_node *exe)
{
	int			pid;
	t_exe_res	res;

	pid = fork();
	if (pid == 0)
	{
		set_unwind_sig();
		set_up_redirection(state, exe);
		exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
		free_executable_node(exe);
		exe->outfd = 1;
		exe->infd = 0;
		res = execute_tree_node(state, exe);
		forward_exit_status(res);
	}
	if (pid < 0)
		critical_error_errno_context("fork");
	free_executable_node(exe);
	return (res_pid(pid));
}

// gives back pid;
t_exe_res	execute_command(t_shell *state, t_executable_node *exe)
{
	t_ast_node	*curr;
	size_t		i;
	int			redir_idx;

	ft_assert(exe->node->children.len >= 1);
	if (((t_ast_node *)exe->node->children.ctx)[0].node_type == AST_SIMPLE_COMMAND)
	{
		exe->node = &((t_ast_node *)exe->node->children.ctx)[0];
		return (execute_simple_command(state, exe));
	}
	ft_assert(((t_ast_node *)exe->node->children.ctx)[0].node_type == AST_SUBSHELL);
	/* ensure redirs vector initialized */
	if (!exe->redirs.ctx)
	{
		vec_init(&exe->redirs);
		exe->redirs.elem_size = sizeof(int);
	}
	i = 0;
	while (++i < exe->node->children.len)
	{
		curr = vec_idx(&exe->node->children, i);
		ft_assert(curr->node_type == AST_REDIRECT);
		if (redirect_from_ast_redir(state, curr, &redir_idx))
			return (res_status(AMBIGUOUS_REDIRECT));
		{ int idx = redir_idx; vec_push(&exe->redirs, &idx); }
	}
	exe->node = vec_idx(&exe->node->children, 0);
	exe->modify_parent_context = true;
	return (execute_subshell(state, exe));
}

// always returns status
t_exe_res	execute_tree_node(t_shell *state, t_executable_node *exe)
{
	t_exe_res		status;
	t_ast_t			t;

	t = exe->node->node_type;
	status = res_status(0);
	if (t == AST_COMMAND_PIPELINE)
		status = execute_pipeline(state, exe);
	else if (t == AST_SIMPLE_LIST || t == AST_COMPOUND_LIST)
		status = execute_simple_list(state, exe);
	else
		ft_assert("Unreachable" == 0);
	set_cmd_status(state, status);
	return (status);
}

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

	/* If stdout is a TTY, set up a capture pipe and a reader child which will
	   forward output to the original stdout and remember the last written byte.
	   After the command tree finishes, if the last byte is not '\n', print
	   "%\n" to the terminal to indicate a missing trailing newline. */
	int monitor_pipe[2] = {-1, -1};
	int status_pipe[2] = {-1, -1};
	int orig_out = -1;
	pid_t reader_pid = -1;
	bool using_monitor = false;

	orig_out = dup(1);
	if (orig_out != -1 && isatty(orig_out))
	{
		if (pipe(monitor_pipe) == 0 && pipe(status_pipe) == 0)
		{
			/* fork the reader BEFORE redirecting stdout */
			reader_pid = fork();
			if (reader_pid == 0)
			{
				/* child: reader */
				if (setpgid(0, 0) == -1) { /* ignore error */ }
				signal(SIGTSTP, SIG_IGN);
				signal(SIGINT, SIG_IGN);
				signal(SIGQUIT, SIG_IGN);
				
				/* Close write ends in reader */
				close(monitor_pipe[1]);
				close(status_pipe[0]);
				
				unsigned char buf[4096];
				ssize_t r;
				unsigned char last = '\n';
				while ((r = read(monitor_pipe[0], buf, sizeof(buf))) > 0)
				{
					ssize_t w = 0;
					while (w < r)
						w += write(orig_out, buf + w, r - w);
					last = buf[r - 1];
				}
				/* send last byte to parent */
				write(status_pipe[1], &last, 1);
				close(status_pipe[1]);
				close(monitor_pipe[0]);
				close(orig_out);
				_exit(0);
			}
			else if (reader_pid > 0)
			{
				/* parent: redirect stdout to pipe */
				if (dup2(monitor_pipe[1], 1) != -1)
				{
					/* close unneeded fds */
					close(monitor_pipe[0]);
					close(monitor_pipe[1]); /* stdout now points to pipe */
					close(status_pipe[1]);
					using_monitor = true;
				}
				else
				{
					/* dup2 failed - cleanup */
					close(monitor_pipe[0]);
					close(monitor_pipe[1]);
					close(status_pipe[0]);
					close(status_pipe[1]);
					/* kill reader */
					kill(reader_pid, SIGKILL);
					waitpid(reader_pid, NULL, 0);
					reader_pid = -1;
				}
			}
			else
			{
				/* fork failed */
				close(monitor_pipe[0]);
				close(monitor_pipe[1]);
				close(status_pipe[0]);
				close(status_pipe[1]);
			}
		}
		else if (monitor_pipe[0] != -1)
		{
			close(monitor_pipe[0]);
			close(monitor_pipe[1]);
		}
	}

	/* gather heredocs and execute the tree */
	if (!g_should_unwind)
		gather_heredocs(state, &state->tree, false);
	if (!g_should_unwind)
		res = execute_tree_node(state, &exe);
	else
		res = res_status(CANCELED);

	/* cleanup monitor */
	if (using_monitor)
	{
		/* Restore stdout - this closes our last reference to the pipe write end */
		dup2(orig_out, 1);
		close(orig_out);

		/* Read last byte reported by reader */
		unsigned char last = '\n';
		ssize_t rr = read(status_pipe[0], &last, 1);
		close(status_pipe[0]);

		/* Wait for reader to finish */
		if (reader_pid > 0)
		{
			int st = 0;
			while (waitpid(reader_pid, &st, 0) == -1 && errno == EINTR)
				;
		}

		/* Print indicator if needed */
		if (rr == 1 && last != '\n')
		{
			write(1, "%\n", 2);
		}
	}
	else
	{
		if (orig_out != -1)
			close(orig_out);
	}

	if (res.c_c)
	{
		if (state->input_method == INP_READLINE)
			ft_eprintf("\n");
		else
			state->should_exit = true;
	}
	state->last_cmd_status_res = res;
}