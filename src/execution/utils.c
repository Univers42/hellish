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

/* Apply one resolved t_redir to the process's fd table.  Three cases:
   close_fd -- just close the fd (2>&- syntax);
   is_dup   -- dup2 src_fd to a different target without closing the source
               (used when the source is a pre-opened read fd like a pipe);
   normal   -- dup2 then close the original so only the canonical fd number
               remains live (the typical file-backed redirect). */
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

/* Look up the t_redir at index `idx` in state->redirects and apply it.
   Out-of-range access is an internal bug (the parser or heredoc collector
   produced a bad index) so we print an error and exit rather than silently
   reading garbage memory. */
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

/* Apply all redirects whose indices are stored in exe->redirs (the
   pre-collected path, used when we already resolved them in the parent
   before forking -- avoids resolving the same file twice). */
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

/* Apply redirects directly from the AST (the lazy path, used in child
   processes where the parent did not pre-resolve).  We walk the node's
   children looking for AST_REDIRECT siblings.  An ambiguous redirect
   (variable expanded to multiple words) causes an error exit. */
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
