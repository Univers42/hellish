/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:00 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"
#include "sys.h"

// returns writable fd
int	ft_mktemp(t_shell *state, t_ast_node *node)
{
	t_redir		ret;
	char		*temp;
	int			wr_fd;
	t_string	fname;

	ret = create_redir(true, 0, true);
	vec_init(&fname);
	vec_push_str(&fname, TMP_HC_DIR);
	if (state->pid)
		vec_push_str(&fname, state->pid);
	vec_push_str(&fname, ULTIMATE_ARG);
	temp = ft_itoa(state->heredoc_idx++);
	vec_push_str(&fname, temp);
	ret.fname = (char *)fname.ctx;
	free(temp);
	wr_fd = open(ret.fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (wr_fd < 0)
		critical_error_errno_context(ret.fname);
	ret.fd = open(ret.fname, O_RDONLY);
	if (ret.fd < 0)
		critical_error_errno_context(ret.fname);
	vec_push(&state->redirects, &ret);
	node->redir_idx = state->redirects.len - 1;
	node->has_redirect = true;
	return (wr_fd);
}

char	*first_non_tab(char *line)
{
	while (*line == '\t')
		line++;
	return (line);
}

void	gather_heredoc(t_shell *state, t_ast_node *node, bool is_pipe)
{
	int				wr_fd;
	t_string		sep;
	t_hdoc	req;

	ft_assert(node->children.len >= 1);
	if (((t_ast_node *)node->children.ctx)[0].token.tt == TT_HEREDOC)
	{
		wr_fd = ft_mktemp(state, node);
		sep = word_to_hrdoc_string(((t_ast_node *)node->children.ctx)[1]);
		ft_assert(sep.ctx != 0);
		req = (t_hdoc){
			.sep = (char *)sep.ctx,
			.expand = !contains_quotes(((t_ast_node *)node->children.ctx)[1]),
			.remove_tabs
			= ft_strncmp(((t_ast_node *)node->children.ctx)[0].token.start,
				STRIP_HEREDOC, 3)
			== 0,
			.is_pipe_heredoc = is_pipe
		};
		write_heredoc(state, wr_fd, &req);
		free(sep.ctx);
	}
}
