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
		critical_error_errno_ctx(ret.fname);
	ret.fd = open(ret.fname, O_RDONLY);
	if (ret.fd < 0)
		critical_error_errno_ctx(ret.fname);
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

/* Defer a heredoc inside a function body: copy its raw body (up to and
   including the delimiter line) out of state->hd_src onto the node. The body is
   re-materialised at call time (materialize_heredoc) because the temp-file /
   redirect index created now would be freed before the function is invoked. */
static bool	is_capture_delim(const char *ls, const char *le, t_string *sep,
				int dash)
{
	size_t	i;

	i = 0;
	if (dash)
		while (ls + i < le && ls[i] == '\t')
			i++;
	return ((size_t)(le - ls - i) == sep->len
		&& ft_strncmp(ls + i, (char *)sep->ctx, sep->len) == 0);
}

bool	capture_heredoc_to_node(t_shell *state, t_ast_node *node)
{
	t_string	sep;
	t_string	body;
	const char	*p;
	const char	*ls;
	int			dash;

	if (!state->hd_src)
		return (false);
	sep = word_to_hrdoc_string(((t_ast_node *)node->children.ctx)[1]);
	dash = (ft_strncmp(((t_ast_node *)node->children.ctx)[0].token.start,
				STRIP_HEREDOC, 3) == 0);
	ls = state->hd_src + state->hd_pos;
	p = ls;
	while (*p)
	{
		ls = p;
		while (*p && *p != '\n')
			p++;
		if (is_capture_delim(ls, p, &sep, dash))
		{
			p += (*p == '\n');
			break ;
		}
		p += (*p == '\n');
	}
	vec_init(&body);
	body.elem_size = 1;
	vec_push_nstr(&body, state->hd_src + state->hd_pos, p - state->hd_src
		- state->hd_pos);
	state->hd_pos = p - state->hd_src;
	return (free(sep.ctx), node->heredoc_body = (char *)body.ctx, true);
}

/* Build the temp file for a deferred heredoc at redirect-setup time, reading the
   stored body back through the hd_src reader so expansion/<<- match a normal
   heredoc. */
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
	req = (t_hdoc){.sep = (char *)sep.ctx, .expand
		= !contains_quotes(((t_ast_node *)node->children.ctx)[1]),
		.remove_tabs = ft_strncmp(((t_ast_node *)node->children.ctx)[0]
			.token.start, STRIP_HEREDOC, 3) == 0, .is_pipe_heredoc = false};
	write_heredoc(state, wr_fd, &req);
	free(sep.ctx);
	state->hd_src = saved_src;
	state->hd_pos = saved_pos;
	return (*redir_idx = node->redir_idx, 0);
}

void	gather_heredoc(t_shell *state, t_ast_node *node, bool is_pipe)
{
	int				wr_fd;
	t_string		sep;
	t_hdoc			req;

	ft_assert(node->children.len >= 1);
	if (((t_ast_node *)node->children.ctx)[0].token.tt == TT_HEREDOC)
	{
		if (state->gather_in_func && capture_heredoc_to_node(state, node))
			return ;
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
