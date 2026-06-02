/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reader2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:05 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/09 23:32:05 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"

void	write_heredoc(t_shell *state, int wr_fd, t_hdoc *req)
{
	while (!req->finished)
		process_line(state, req);
	if (req->full_file.len)
	{
		if (!vec_ensure_space_n(&req->full_file, 1))
			return ;
		((char *)req->full_file.ctx)[req->full_file.len] = '\0';
		ft_assert(write_to_file((char *)req->full_file.ctx, wr_fd) == 0);
	}
	close(wr_fd);
	free(req->full_file.ctx);
}

bool	contains_quotes(t_ast_node node)
{
	size_t	i;

	if (node.node_type == AST_TOKEN
		&& (node.token.tt == TT_DQENVVAR || node.token.tt == TT_DQWORD
			|| node.token.tt == TT_SQWORD))
		return (true);
	i = -1;
	while (++i < node.children.len)
		if (contains_quotes(((t_ast_node *)node.children.ctx)[i]))
			return (true);
	return (false);
}
