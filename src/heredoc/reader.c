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

/* When executing a string (command substitution, eval, source) the heredoc
   bodies were extracted up front into state->hd_src; serve lines from there
   instead of the live input stream. Returns 0 at end-of-source (EOF-like). */
static int	read_hd_src_line(t_shell *state, t_string *out)
{
	size_t	start;

	if (!state->hd_src[state->hd_pos])
		return (0);
	start = state->hd_pos;
	while (state->hd_src[state->hd_pos] && state->hd_src[state->hd_pos] != '\n')
		state->hd_pos++;
	if (state->hd_src[state->hd_pos] == '\n')
		state->hd_pos++;
	vec_push_nstr(out, state->hd_src + start, state->hd_pos - start);
	return (4);
}

/* Read one line of heredoc body.  Source priority: if hd_src is set (we
   are executing a string -- eval/source/cmdsub) we serve from the
   pre-extracted body stream; otherwise we call the readline layer.  EOF
   (stat==0) or Ctrl-D (stat==2) trigger the POSIX "delimited by EOF"
   warning and mark req->finished so the caller loop terminates.  A stat
   of 4 from read_hd_src_line means "got a line". */
bool	get_line_heredoc(t_shell *state,
		t_hdoc *req, t_string *alloc_line)
{
	int		stat;
	char	*prompt;

	if (req->is_pipe_heredoc)
		prompt = "pipe heredoc> ";
	else
		prompt = "heredoc> ";
	vec_init(alloc_line);
	if (state->hd_src)
		stat = read_hd_src_line(state, alloc_line);
	else
		stat = buff_readline(state, alloc_line, prompt);
	state->rl.has_finished = false;
	if (stat == 0)
		ft_eprintf("%s: warning: here-document at"
			" line %i delimited by end-of-file (wanted `%s')\n",
			state->ctx, state->rl.line, req->sep);
	if (stat == 0 || stat == 2)
	{
		req->finished = true;
		return (true);
	}
	return (false);
}

/* Return true when alloc_line is exactly the delimiter.  We check both
   NUL-terminated (write path) and newline-terminated (readline path)
   forms so the comparison works regardless of whether the caller appended
   a trailing newline.  The leading-newline guard avoids matching the
   delimiter in the middle of a line (POSIX: delimiter must be on its own
   line starting at column 0). */
bool	is_sep(t_hdoc *req, t_string *alloc_line)
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

/* <<- : strip leading TABS (not spaces) from a heredoc line. */
static void	strip_leading_tabs(t_string *l)
{
	size_t	i;

	i = 0;
	while (i < l->len && ((char *)l->ctx)[i] == '\t')
		i++;
	if (i == 0)
		return ;
	ft_memmove(l->ctx, (char *)l->ctx + i, l->len - i + 1);
	l->len -= i;
}

/* Read one heredoc line, strip leading tabs if <<- mode, check for the
   delimiter, and either expand+append the line into req->full_file or
   append it verbatim (when the delimiter was quoted).  Sets req->finished
   on the delimiter line so write_heredoc exits its loop.  The alloc_line
   buffer is freed here after we are done with it. */
void	process_line(t_shell *state, t_hdoc *req)
{
	t_string	alloc_line;
	char		*line;

	if (get_line_heredoc(state, req, &alloc_line))
		return ;
	if (req->remove_tabs)
		strip_leading_tabs(&alloc_line);
	if (is_sep(req, &alloc_line))
		return (xfree(alloc_line.ctx), (void)(req->finished = true));
	line = (char *)alloc_line.ctx;
	if (!req->full_file.ctx)
	{
		vec_init(&req->full_file);
		req->full_file.elem_size = 1;
	}
	if (req->expand)
		expand_line(state, &req->full_file, line);
	else if (line)
		vec_push_str(&req->full_file, line);
	xfree(alloc_line.ctx);
}
