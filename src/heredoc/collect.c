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

/* Create the temp file for a heredoc.  The name is composed from a stable
   prefix (TMP_HC_DIR), the shell PID (so concurrent shell instances do not
   collide), and the ULTIMATE_ARG sentinel plus a per-command integer index
   (heredoc_idx++) so multiple heredocs in one command each get unique
   files.  The open() call gets both a write-fd (returned to the caller for
   writing the expanded body) and a read-fd (stored in the t_redir so the
   child can use it as stdin).  Both fds refer to the same file; unlinking
   is not done here -- the temp file persists for the life of the command. */
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

/* Skip leading TABs from a heredoc line (<<- stripping).  POSIX says only
   TABs are stripped, not spaces, so we increment while *line == '\t'. */
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
/* Check if the line [ls, le) is exactly the delimiter string (with
   leading TABs stripped when dash/<<- mode is active).  The comparison
   is byte-exact: a delimiter with trailing whitespace or a partial match
   does not count. */
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

/* Walk through `src` line by line until we find the delimiter or run out
   of input.  Returns a pointer just past the delimiter line (including its
   trailing newline), or a pointer to the NUL if no delimiter was found.
   Used by capture_heredoc_to_node to slice out one heredoc body from
   state->hd_src without copying. */
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

/* Defer a heredoc's body: instead of writing it to a temp file now, copy
   the raw body (up to and including the delimiter line) from state->hd_src
   onto node->heredoc_body.  materialize_heredoc() will create the actual
   temp file and do expansion at execution time.  This is critical inside
   function bodies and loops: the temp-file fd created at parse time would
   be freed/closed before the function is ever called. */
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
