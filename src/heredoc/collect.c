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
	xfree(temp);
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

static const char	*scan_to_delim(const char *src, t_string *sep, int dash)
{
	const char	*p;
	const char	*ls;

	p = src;
	while (*p)
	{
		ls = p;
		while (*p && *p != '\n')
			p++;
		if (is_capture_delim(ls, p, sep, dash))
		{
			p += (*p == '\n');
			return (p);
		}
		p += (*p == '\n');
	}
	return (p);
}

bool	capture_heredoc_to_node(t_shell *state, t_ast_node *node)
{
	t_string	sep;
	t_string	body;
	const char	*end;
	int			dash;

	if (!state->hd_src)
		return (false);
	sep = word_to_hrdoc_string(((t_ast_node *)node->children.ctx)[1]);
	dash = (ft_strncmp(((t_ast_node *)node->children.ctx)[0].token.start,
				STRIP_HEREDOC, 3) == 0);
	end = scan_to_delim(state->hd_src + state->hd_pos, &sep, dash);
	vec_init(&body);
	body.elem_size = 1;
	vec_push_nstr(&body, state->hd_src + state->hd_pos,
		end - state->hd_src - state->hd_pos);
	state->hd_pos = end - state->hd_src;
	xfree(sep.ctx);
	node->heredoc_body = (char *)body.ctx;
	return (true);
}
