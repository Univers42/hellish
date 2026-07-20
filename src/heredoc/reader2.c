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

/* Consume the heredoc body line by line (via process_line) until the
   delimiter is seen or EOF is hit.  When done, NUL-terminate the buffer
   and hand it to hdoc_attach_backing, which fills the redirect slot at
   `redir_idx` with a pipe (small body) or an unlinked temp file (large
   body).  The redirect teardown closes the resulting fd exactly once at
   command end. */
void	write_heredoc(t_shell *state, int redir_idx, t_hdoc *req)
{
	state->rl.line_exact = true;
	while (!req->finished)
		process_line(state, req);
	state->rl.line_exact = false;
	if (req->full_file.len)
	{
		if (!vec_ensure_space_n(&req->full_file, 1))
			return ;
		((char *)req->full_file.ctx)[req->full_file.len] = '\0';
	}
	hdoc_attach_backing(state, redir_idx, &req->full_file);
	xfree(req->full_file.ctx);
}

/* Recursively check whether an AST node or any of its descendants contains
   a quoted token (single-quoted, double-quoted word, or double-quoted
   variable reference).  Used to decide whether to suppress expansion in a
   heredoc body: `<<"EOF"` has a quoted delimiter and must not expand $. */
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
