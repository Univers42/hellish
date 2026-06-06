/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_simple_cmd.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:22 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sh_input.h"

bool	is_readonly_var(t_shell *state, const char *key);
void	exit_clean(t_shell *state, int code);

/* Prepare an executable_cmd for a new simple command.  pre_assigns holds
   VAR=val assignments that precede the command name; argv borrows its
   storage from the shell's per-call-depth argv pool to avoid malloc on
   every command — see argv_pool_acquire for the recycling logic. */
static void	init_executable_cmd(t_shell *state, t_executable_cmd *ret)
{
	*ret = (t_executable_cmd){};
	vec_init(&ret->pre_assigns);
	ret->pre_assigns.elem_size = sizeof(t_env);
	argv_pool_acquire(state, ret);
}

/* If the simple command is assignment-only (no command name, only VAR=val
   words), transfer the pre_assigns into the current shell environment.
   POSIX says "VAR=x" alone is equivalent to a shell assignment.  Readonly
   variable violations are caught here and result in a non-zero exit status
   (or exit in non-interactive mode). */
static void	apply_pre_assigns_if_assignment_only(t_shell *state,
												t_executable_cmd *ret)
{
	t_env	tmp;

	if (ret->argv.len == 0 && ret->pre_assigns.len > 0)
	{
		while (ret->pre_assigns.len)
		{
			tmp = *(t_env *)vec_pop(&ret->pre_assigns);
			if (tmp.key && is_readonly_var(state, tmp.key))
			{
				ft_eprintf("%s: %s: readonly variable\n", state->ctx, tmp.key);
				xfree(tmp.key);
				xfree(tmp.value);
				if (state->metinp != INP_RL)
					exit_clean(state, 127);
				state->last_cmd_st_exe = (t_execution_state){.status = 1};
			}
			else
				env_set(&state->env, tmp);
		}
		xfree(ret->pre_assigns.ctx);
		vec_init(&ret->pre_assigns);
	}
}

/* Expand all children of a simple command AST node into an executable_cmd.
   The children are: AST_WORD (command name / arg), AST_ASSIGNMENT_WORD
   (pre-command VAR=val), AST_REDIRECT, and AST_PROC_SUB.  Each is routed
   through process_simple_child.  Assignment-only commands (no name found)
   are handled by apply_pre_assigns_if_assignment_only at the end. */
int	expand_simple_command(t_shell *state, t_ast_node *node,
						t_executable_cmd *ret, t_vec_int *redirects)
{
	t_expander_simple_cmd	exp;

	if (node == NULL || node->children.len == 0 || node->children.ctx == NULL)
		return (1);
	if (node->children.elem_size == 0)
		node->children.elem_size = sizeof(t_ast_node);
	exp = (t_expander_simple_cmd){0};
	init_executable_cmd(state, ret);
	while (exp.i < node->children.len)
	{
		exp.curr = (t_ast_node *)vec_idx(&node->children, exp.i);
		exp.exit_stat = process_simple_child(state, &exp, ret, redirects);
		if (exp.exit_stat)
			return (exp.exit_stat);
		exp.i++;
	}
	apply_pre_assigns_if_assignment_only(state, ret);
	return (0);
}
