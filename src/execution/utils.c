/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 16:54:50 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

void set_last_underscore_var(t_shell *state, t_executable_cmd *cmd)
{
	char *last;

	if (cmd->argv.len > 0)
	{
		last = ((char **)cmd->argv.ctx)[cmd->argv.len - 1];
		if (last)
			env_set(&state->env,
					env_create(ft_strdup("_"), ft_strdup(last), true));
	}
}

void update_underscore_var(t_shell *state, t_executable_cmd *cmd)
{
	set_last_underscore_var(state, cmd);
}

void apply_redir(t_shell *state, int idx)
{
	t_redir redir;

	if (idx < 0 || !state->redirects.ctx || (size_t)idx >= state->redirects.len)
	{
		ft_eprintf("%s: internal error: invalid redirect index %d\n",
				   state->context ? state->context : "minishell", idx);
		_exit(1);
	}
	redir = ((t_redir *)state->redirects.ctx)[(size_t)idx];
	dup2(redir.fd, redir.src_fd);
	close(redir.fd);
}

void apply_redirs_from_vec(t_shell *state, t_executable_node *exe)
{
	size_t i;

	i = 0;
	while (i < exe->redirs.len)
	{
		int idx = *(int *)vec_idx(&exe->redirs, i++);
		apply_redir(state, idx);
	}
}

void apply_redirs_from_ast(t_shell *state, t_executable_node *exe)
{
	size_t i;
	t_ast_node *curr;
	int idx;

	for (i = 1; i < exe->node->children.len; ++i)
	{
		curr = (t_ast_node *)vec_idx(&exe->node->children, i);
		if (curr->node_type != AST_REDIRECT)
			continue;
		if (redirect_from_ast_redir(state, curr, &idx))
		{
			ft_eprintf("%s: ambiguous redirect\n", state->context);
			_exit(1);
		}
		apply_redir(state, idx);
	}
}
