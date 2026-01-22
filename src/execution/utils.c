/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 15:17:11 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

void update_underscore_var(t_shell *state, t_executable_cmd *cmd)
{
	if (cmd->argv.len > 0)
	{
		char *last = ((char **)cmd->argv.ctx)[cmd->argv.len - 1];
		if (last)
			env_set(&state->env, (t_env){
									 .exported = true,
									 .key = ft_strdup("_"),
									 .value = ft_strdup(last)});
	}
}


void set_up_redirection(t_shell *state, t_executable_node *exe)
{
	t_redir redir;
	size_t i;

	if (exe->next_infd != -1)
		close(exe->next_infd);
	dup2(exe->outfd, 1);
	dup2(exe->infd, 0);

	/* nothing to do */
	if (exe->redirs.len == 0)
		return;

	/* Case A: redir indices available in exe->redirs.ctx */
	if (exe->redirs.ctx)
	{
		i = 0;
		while (i < exe->redirs.len)
		{
			int idx = *(int *)vec_idx(&exe->redirs, i++);
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
		return;
	}

	/* Case B: no redirs buffer available — fallback: scan AST children for redirects */
	if (exe->node)
	{
		for (i = 1; i < exe->node->children.len; ++i)
		{
			t_ast_node *curr = (t_ast_node *)vec_idx(&exe->node->children, i);
			if (curr->node_type != AST_REDIRECT)
				continue;
			int idx;
			if (redirect_from_ast_redir(state, curr, &idx))
			{
				ft_eprintf("%s: ambiguous redirect\n", state->context);
				_exit(1);
			}
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
		return;
	}

	ft_eprintf("%s: internal error: redirects present but no redirect data\n",
			   state->context ? state->context : "minishell");
	_exit(1);
}