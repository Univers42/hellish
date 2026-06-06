/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   collect2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/09 23:32:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"
#include "sys.h"
#include "sh_input.h"

/* Same-line heredoc in -c / script mode: when gather runs, the body still sits
   ahead in state->rl.buff (state->input held only the first line, so
   split_heredocs could not pre-extract and hd_src is NULL). Point hd_src at the
   un-read remainder of rl.buff, capture the RAW body onto the node (so
   materialize_heredoc expands it at execution time, after any preceding
   same-line assignment), then advance rl.cursor past what we consumed so the
   REPL does not re-read the body lines as commands. */
bool	capture_heredoc_from_buff(t_shell *state, t_ast_node *node)
{
	char	*saved_src;
	size_t	saved_pos;
	bool	ok;

	if (state->metinp != INP_ARG && state->metinp != INP_FILE)
		return (false);
	if (!state->rl.buff.ctx || state->rl.cursor >= state->rl.buff.len)
		return (false);
	saved_src = state->hd_src;
	saved_pos = state->hd_pos;
	state->hd_src = (char *)state->rl.buff.ctx + state->rl.cursor;
	state->hd_pos = 0;
	ok = capture_heredoc_to_node(state, node);
	state->rl.cursor += state->hd_pos;
	state->hd_src = saved_src;
	state->hd_pos = saved_pos;
	return (ok);
}

static t_hdoc	build_hdoc_req(t_ast_node *node, bool is_pipe, t_string *sep)
{
	t_hdoc	req;

	req.sep = (char *)sep->ctx;
	req.expand = !contains_quotes(((t_ast_node *)node->children.ctx)[1]);
	req.remove_tabs = (ft_strncmp(
				((t_ast_node *)node->children.ctx)[0].token.start,
				STRIP_HEREDOC, 3) == 0);
	req.is_pipe_heredoc = is_pipe;
	req.finished = false;
	req.full_file = (t_string){0};
	return (req);
}

/* Build temp file for a deferred heredoc at redirect-setup time. */
int	materialize_heredoc(t_shell *state, t_ast_node *node, int *redir_idx)
{
	char		*saved_src;
	size_t		saved_pos;
	int			wr_fd;
	t_string	sep;
	t_hdoc		req;

	saved_src = state->hd_src;
	saved_pos = state->hd_pos;
	state->hd_src = node->heredoc_body;
	state->hd_pos = 0;
	wr_fd = ft_mktemp(state, node);
	sep = word_to_hrdoc_string(((t_ast_node *)node->children.ctx)[1]);
	req = build_hdoc_req(node, false, &sep);
	write_heredoc(state, wr_fd, &req);
	xfree(sep.ctx);
	state->hd_src = saved_src;
	state->hd_pos = saved_pos;
	*redir_idx = node->redir_idx;
	return (0);
}

void	gather_heredoc(t_shell *state, t_ast_node *node, bool is_pipe)
{
	int			wr_fd;
	t_string	sep;
	t_hdoc		req;

	ft_assert(node->children.len >= 1);
	if (((t_ast_node *)node->children.ctx)[0].token.tt != TT_HEREDOC)
		return ;
	if (state->gather_in_func && capture_heredoc_to_node(state, node))
		return ;
	wr_fd = ft_mktemp(state, node);
	sep = word_to_hrdoc_string(((t_ast_node *)node->children.ctx)[1]);
	ft_assert(sep.ctx != 0);
	req = build_hdoc_req(node, is_pipe, &sep);
	write_heredoc(state, wr_fd, &req);
	xfree(sep.ctx);
}
