/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:05 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"

bool	get_line_heredoc(t_shell *state,
		t_heredoc_req *req, t_string *alloc_line)
{
	int		stat;
	char	*prompt;

	if (req->is_pipe_heredoc)
		prompt = "pipe heredoc> ";
	else
		prompt = "heredoc> ";
	vec_init(alloc_line);
	stat = buff_readline(state, alloc_line, prompt);
	state->readline_buff.has_finished = false;
	if (stat == 0)
		ft_eprintf("%s: warning: here-document at"
			" line %i delimited by end-of-file (wanted `%s')\n",
			state->context, state->readline_buff.line, req->sep);
	if (stat == 0 || stat == 2)
	{
		req->finished = true;
		return (true);
	}
	return (false);
}

bool	is_sep(t_heredoc_req *req, t_string *alloc_line)
{
	size_t	sep_len;

	sep_len = ft_strlen(req->sep);
	if ((req->full_file.len == 0
			|| ((char *)req->full_file.ctx)[req->full_file.len - 1] == '\n'))
	{
		if (ft_strcmp((char *)alloc_line->ctx, req->sep) == 0)
			return (true);
		else if (((char *)alloc_line->ctx)[alloc_line->len - 1] == '\n'
			&& sep_len + 1 == alloc_line->len
			&& ft_strncmp((char *)alloc_line->ctx, req->sep, sep_len) == 0)
			return (true);
	}
	return (false);
}

// should brake
void	process_line(t_shell *state, t_heredoc_req *req)
{
	t_string	alloc_line;
	char		*line;

	if (get_line_heredoc(state, req, &alloc_line))
		return ;
	if (is_sep(req, &alloc_line))
		return (free(alloc_line.ctx), (void)(req->finished = true));
	if (req->remove_tabs)
		line = first_non_tab((char *)alloc_line.ctx);
	else
		line = (char *)alloc_line.ctx;
	if (req->expand)
		expand_line(state, &req->full_file, line);
	else
		vec_push_str(&req->full_file, line);
	free(alloc_line.ctx);
}

void	write_heredoc(t_shell *state, int wr_fd, t_heredoc_req *req)
{
	while (!req->finished)
	{
		process_line(state, req);
	}
	if (req->full_file.len)
	{
		if (!vec_ensure_space_n(&req->full_file, 1))
			return ;
		((char *)req->full_file.ctx)[req->full_file.len] = '\0';
		ft_assert(write_to_file((char *)req->full_file.ctx, wr_fd) == 0);
	}
	(close(wr_fd), free(req->full_file.ctx));
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
