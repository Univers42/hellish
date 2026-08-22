/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_history4.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>

/* history -a|-n|-r|-w [file] -- the four options that move entries between
   the list and a file. All four used to be parsed as "no count", i.e. as a
   request to print the whole history: issue #42. */

/* Which file. An explicit operand wins, then $HISTFILE (bash's rule), then
   this session's own history file. A script that has neither has nothing
   sensible to write to, and bash 5.3 says so with a status of 1 rather
   than quietly picking a file for you. */
static char	*hist_target(t_shell *state, t_vec argv, int first)
{
	char	*v;

	if (first < (int)argv.len)
		return (ft_strdup(((char **)argv.ctx)[first]));
	v = env_expand(state, "HISTFILE");
	if (v && *v)
		return (ft_strdup(v));
	if (state->hist.hist_active)
		return (get_hist_file_path(state));
	return (NULL);
}

/* True when -a's target is the file this session is ALREADY streaming into.
   hellish appends each command as it is entered (bash defers to exit or to
   an explicit `history -a`), so writing them again here would duplicate
   every line -- and with `PROMPT_COMMAND='history -a'`, once per prompt,
   for ever. Mark them saved and do nothing. */
static bool	hist_streamed(t_shell *state, t_vec argv, t_histopt *o)
{
	char	*v;

	if (o->first < (int)argv.len || state->hist.append_fd < 0)
		return (false);
	v = env_expand(state, "HISTFILE");
	return (!v || !*v);
}

/* -w (truncate) and -a (append): write entries [from, end) to path. */
static int	hist_write(t_shell *state, const char *path, size_t from, int fl)
{
	int		fd;
	char	*enc;

	fd = open(path, fl, 0600);
	if (fd < 0)
		return (ft_eprintf("%s: history: %s: %s\n", state->ctx, path,
				strerror(errno)), 1);
	while (from < state->hist.hist_cmds.len)
	{
		enc = (char *)encode_cmd_hist(
				((char **)state->hist.hist_cmds.ctx)[from++]).ctx;
		if (write(fd, enc, ft_strlen(enc)) < 0)
			errno = 0;
		xfree(enc);
	}
	close(fd);
	return (0);
}

/* -r (whole file) and -n (only what a previous read did not take): decode
   path and append its entries to the list. */
static int	hist_read(t_shell *state, const char *path, size_t skip)
{
	t_string	buf;
	t_vec		got;
	size_t		i;
	int			fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (ft_eprintf("%s: history: %s: %s\n", state->ctx, path,
				strerror(errno)), 1);
	vec_init(&buf);
	buf.elem_size = 1;
	vec_append_fd(fd, &buf);
	close(fd);
	got = parse_hist_file(buf);
	xfree(buf.ctx);
	i = 0;
	while (i < got.len)
	{
		if (i++ >= skip)
			hist_push(state, ((char **)got.ctx)[i - 1]);
		else
			xfree(((char **)got.ctx)[i - 1]);
	}
	state->hist.readmark = got.len;
	return (xfree(got.ctx), 0);
}

/* Dispatch the one file operation the parse settled on. */
int	hist_fileop(t_shell *state, t_vec argv, t_histopt *o)
{
	char	*path;
	size_t	from;
	int		st;

	path = hist_target(state, argv, o->first);
	if (!path)
		return (ft_eprintf("%s: history: HISTFILE: parameter null or not"
				" set\n", state->ctx), 1);
	from = state->hist.appended;
	if (from > state->hist.hist_cmds.len)
		from = state->hist.hist_cmds.len;
	st = 0;
	if (o->fileop == 'w')
		st = hist_write(state, path, 0, O_WRONLY | O_CREAT | O_TRUNC);
	else if (o->fileop == 'a' && !hist_streamed(state, argv, o))
		st = hist_write(state, path, from, O_WRONLY | O_CREAT | O_APPEND);
	else if (o->fileop == 'r')
		st = hist_read(state, path, 0);
	else if (o->fileop == 'n')
		st = hist_read(state, path, state->hist.readmark);
	if (o->fileop == 'a')
		state->hist.appended = state->hist.hist_cmds.len;
	return (xfree(path), st);
}
