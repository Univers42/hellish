/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 01:15:14 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

void	set_last_underscore_var(t_shell *state, t_executable_cmd *cmd)
{
	char	*last;

	if (cmd->argv.len > 0)
	{
		last = ((char **)cmd->argv.ctx)[cmd->argv.len - 1];
		if (last)
			env_set(&state->env,
				env_create(ft_strdup(ULTIMATE_ARG), ft_strdup(last), true));
	}
}

void	update_underscore_var(t_shell *state, t_executable_cmd *cmd)
{
	set_last_underscore_var(state, cmd);
}

void	apply_redir(t_shell *state, int idx)
{
	t_redir	redir;

	if (idx < 0 || !state->redirects.ctx || (size_t)idx >= state->redirects.len)
	{
		if (state->context)
			ft_eprintf(MSG_INT_ERR_REDIR_IDX, state->context, idx);
		else
			ft_eprintf(MSG_INT_ERR_REDIR_IDX, NAME, idx);
		exit(EXIT_GENERAL_ERR);
	}
	redir = ((t_redir *)state->redirects.ctx)[(size_t)idx];
	if (redir.close_fd)
	{
		close(redir.src_fd);
		return ;
	}
	if (redir.fd != redir.src_fd)
	{
		dup2(redir.fd, redir.src_fd);
		close(redir.fd);
	}
}

void	apply_redirs_from_vec(t_shell *state, t_executable_node *exe)
{
	size_t	i;
	int		idx;

	i = 0;
	while (i < exe->redirs.len)
	{
		idx = *(int *)vec_idx(&exe->redirs, i++);
		apply_redir(state, idx);
	}
}

void	apply_redirs_from_ast(t_shell *state, t_executable_node *exe)
{
	size_t		i;
	t_ast_node	*curr;
	int			idx;

	i = 0;
	while (++i < exe->node->children.len)
	{
		curr = (t_ast_node *)vec_idx(&exe->node->children, i);
		if (curr->node_type != AST_REDIRECT)
			continue ;
		if (redirect_from_ast_redir(state, curr, &idx))
		{
			ft_eprintf(MSG_AMBIGUOUS_REDIR, state->context);
			exit(EXIT_GENERAL_ERR);
		}
		apply_redir(state, idx);
	}
}
