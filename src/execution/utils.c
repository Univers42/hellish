/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:25:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

static void	apply_redir_now(t_redir redir)
{
	if (redir.close_fd)
	{
		close(redir.src_fd);
		return ;
	}
	if (redir.is_dup)
	{
		dup2(redir.fd, redir.src_fd);
		return ;
	}
	if (redir.fd != redir.src_fd)
	{
		dup2(redir.fd, redir.src_fd);
		close(redir.fd);
	}
}

void	apply_redir(t_shell *state, int idx)
{
	if (idx < 0 || !state->redirects.ctx
		|| (size_t)idx >= state->redirects.len)
	{
		if (state->ctx)
			ft_eprintf(MSG_INT_ERR_REDIR_IDX, state->ctx, idx);
		else
			ft_eprintf(MSG_INT_ERR_REDIR_IDX, NAME, idx);
		exit(EXIT_GENERAL_ERR);
	}
	apply_redir_now(((t_redir *)state->redirects.ctx)[(size_t)idx]);
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
			ft_eprintf(MSG_AMBIGUOUS_REDIR, state->ctx);
			exit(EXIT_GENERAL_ERR);
		}
		apply_redir(state, idx);
	}
}
