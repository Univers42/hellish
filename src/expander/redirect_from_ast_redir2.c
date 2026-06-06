/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redirect_from_ast_redir2.c                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

int	get_default_src_fd(t_tt tt);
int	try_create_redir(t_shell *state, t_ast_node *curr, t_tt tt, int src_fd);

/* Read the fd number stored in the operator-token's leading digits (child 0
   of the redirect node).  The lexer packs "2>" as a token whose text starts
   with the '2'; we parse those digits and return the fd, falling back to the
   operator's default fd (0 for reads, 1 for writes) if no digits are found. */
int	get_src_fd(t_ast_node *curr, t_tt tt)
{
	t_ast_node	*first;
	const char	*s;
	int			i;
	int			fd;

	if (curr->children.len == 0)
		return (get_default_src_fd(tt));
	first = &((t_ast_node *)curr->children.ctx)[0];
	if (first->node_type != AST_TOKEN)
		return (get_default_src_fd(tt));
	s = first->token.start;
	i = 0;
	fd = 0;
	while (i < first->token.len && ft_isdigit((unsigned char)s[i]))
	{
		fd = fd * 10 + (s[i] - '0');
		i++;
	}
	if (i == 0)
		return (get_default_src_fd(tt));
	return (fd);
}

/* Top-level redirect processor for one AST_REDIRECT node.  Heredocs are
   handled specially: if the heredoc body is already materialised (has_redirect)
   we just return its cached index; if not yet materialised, materialize_heredoc
   is called.  All other redirect types go through try_create_redir which opens
   the file and registers the fd in state->redirects. */
int	redirect_from_ast_redir(t_shell *state, t_ast_node *curr, int *redir_idx)
{
	t_token	op_tok;
	t_tt	tt;
	int		src_fd;

	ft_assert(curr->node_type == AST_REDIRECT);
	op_tok = ((t_ast_node *)curr->children.ctx)[0].token;
	tt = op_tok.tt;
	if (tt == TT_HEREDOC && curr->heredoc_body)
		return (materialize_heredoc(state, curr, redir_idx));
	if (curr->has_redirect && tt == TT_HEREDOC)
	{
		*redir_idx = curr->redir_idx;
		return (0);
	}
	src_fd = parse_src_fd(tt, op_tok);
	if (try_create_redir(state, curr, tt, src_fd) < 0)
		return (-1);
	*redir_idx = curr->redir_idx;
	return (0);
}
