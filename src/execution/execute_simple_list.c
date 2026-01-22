/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_list.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Reap any zombie background children (non-blocking) */
void reap_background_children(void)
{
	int status;
	
	/* Keep reaping until no more zombies */
	while (waitpid(-1, &status, WNOHANG) > 0)
		;
}

bool	should_execute(t_exe_res prev_status, t_tt prev_op)
{
	if (prev_status.c_c)
		return (false);
	ft_assert(prev_status.status != -1);
	ft_assert(prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE
		|| prev_op == TT_AND || prev_op == TT_OR || prev_op == TT_AMPERSAND);
	if (prev_op == TT_SEMICOLON || prev_op == TT_NEWLINE || prev_op == TT_AMPERSAND)
		return (true);
	if (prev_op == TT_AND && prev_status.status == 0)
		return (true);
	if (prev_op == TT_OR && prev_status.status != 0)
		return (true);
	return (false);
}

/* Find the next & or ; operator in the children list starting from index i.
   Returns the index of the operator, or children.len if not found.
   Also sets *found_amp to true if & was found before ; */
size_t find_next_separator(t_ast_node *node, size_t start, bool *found_amp)
{
	size_t i;
	t_ast_node *child;

	*found_amp = false;
	i = start;
	while (i < node->children.len)
	{
		child = &((t_ast_node *)node->children.ctx)[i];
		if (child->node_type == AST_TOKEN)
		{
			if (child->token.tt == TT_AMPERSAND)
			{
				*found_amp = true;
				return i;
			}
			if (child->token.tt == TT_SEMICOLON || child->token.tt == TT_NEWLINE)
				return i;
		}
		i++;
	}
	return node->children.len;
}

/* Execute a range of children [start, end) as a sequence in the current process.
   This handles && and || chaining within the range. */
t_exe_res execute_range(t_shell *state, t_executable_node *exe,
							   size_t start, size_t end)
{
	t_exe_res status;
	t_tt op;
	t_executable_node exe_curr;
	size_t i;
	t_ast_node *child;

	status = res_status(0);
	op = TT_SEMICOLON;
	i = start;
	while (i < end)
	{
		child = &((t_ast_node *)exe->node->children.ctx)[i];
		
		/* Skip operator tokens - update op and continue */
		if (child->node_type == AST_TOKEN)
		{
			op = child->token.tt;
			i++;
			continue;
		}
		
		/* Execute command if conditions are met */
		if (should_execute(status, op))
		{
			exe_curr = *exe;
			exe_curr.node = child;
			status = execute_tree_node(state, &exe_curr);
			/* Wait for the child process to complete */
			if (status.pid != -1)
				exe_res_set_status(&status);
			ft_assert(status.status != -1);
		}
		i++;
	}
	return status;
}

/* Execute a command sequence in the background */
t_exe_res execute_range_background(t_shell *state, t_executable_node *exe,
										  size_t start, size_t end)
{
	pid_t pid;
	int null_fd;

	pid = fork();
	if (pid == 0)
	{
		/* Child process: detach from terminal and execute the range */
		setpgid(0, 0);
		
		/* Redirect stdin from /dev/null */
		null_fd = open("/dev/null", O_RDONLY);
		if (null_fd >= 0)
		{
			dup2(null_fd, STDIN_FILENO);
			close(null_fd);
		}
		
		/* Ignore job control signals */
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		
		/* Execute the command sequence (execute_range waits for children) */
		t_exe_res res = execute_range(state, exe, start, end);
		
		/* Exit with the final status */
		_exit(res.status);
	}
	if (pid < 0)
		critical_error_errno_context("fork");
	
	/* Parent: print job info and continue without waiting */
	state->bg_job_count++;
	ft_printf("[%d] %d\n", state->bg_job_count, pid);
	return res_status(0);
}

t_exe_res execute_simple_list(t_shell *state, t_executable_node *exe)
{
	t_exe_res status;
	size_t i;
	size_t sep_idx;
	bool is_background;

	/* Reap any zombie background children before starting */
	reap_background_children();
	
	status = res_status(0);
	i = 0;
	
	while (i < exe->node->children.len)
	{
		/* Find the next separator (& or ;) */
		sep_idx = find_next_separator(exe->node, i, &is_background);
		
		if (is_background)
		{
			/* Execute the range [i, sep_idx) in background */
			status = execute_range_background(state, exe, i, sep_idx);
		}
		else
		{
			/* Execute the range [i, sep_idx) in foreground */
			status = execute_range(state, exe, i, sep_idx);
		}
		
		/* Skip past the separator */
		i = sep_idx + 1;
	}
	
	/* Reap any zombie background children after execution */
	reap_background_children();
	
	return status;
}
