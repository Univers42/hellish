/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read3.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>
#include <poll.h>

/* -t SECS as milliseconds. Accepts the fractional form bash accepts
   (`-t 0.1`) to three digits; anything past that is dropped rather than
   rounded, which errs toward waiting slightly less, never longer. */
long	rd_secs_ms(const char *s)
{
	long	ms;
	int		i;
	int		scale;

	ms = ft_atol(s) * 1000;
	i = 0;
	while (s[i] && s[i] != '.')
		i++;
	if (!s[i])
		return (ms);
	i++;
	scale = 100;
	while (s[i] >= '0' && s[i] <= '9' && scale > 0)
	{
		ms += (s[i] - '0') * scale;
		scale /= 10;
		i++;
	}
	return (ms);
}

/* -t: wait for stdin to become readable. Returns 0 when there is input (or
   no timeout was asked for), 1 when the deadline passed with nothing there.
     bash reports a timeout with a status ABOVE 128 and leaves the named
   variables untouched, so a caller can tell "no input yet" from "read an
   empty line" -- the two are otherwise indistinguishable and scripts branch
   on the difference. */
int	rd_wait_input(t_rdopt *o)
{
	struct pollfd	pfd;

	if (o->tmo_ms < 0)
		return (0);
	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN;
	pfd.revents = 0;
	return (poll(&pfd, 1, (int)o->tmo_ms) <= 0);
}

/* Route the completed line to its destination: -a fills the named array,
   no variable names sends the whole line to $REPLY, otherwise the line is
   IFS-split across the named variables. rd_set_var takes ownership of the
   line; the array/word paths copy fields out of it, so it is freed here. */
static void	rd_dispatch(t_shell *state, t_vec argv, char *line, t_rdopt *o)
{
	if (o->aname)
	{
		rd_assign_array(state, line, o);
		xfree(line);
	}
	else if (o->first >= argv.len)
		rd_set_var(state, "REPLY", line);
	else
	{
		assign_words(state, line, argv, o);
		xfree(line);
	}
}

/* read [-r] [-n N] [-N N] [-d C] [-t S] [-p PROMPT] [-a NAME] [var ...]:
   read one logical line from stdin and split it into variables. If no
   variable names are given, the whole line goes into $REPLY (POSIX).
   Returns 1 on EOF even if some data was read (mimics bash), so `while read
   line; do …; done` processes the last line before stopping even when the
   file lacks a trailing newline.
     -N reads a fixed byte count and must NOT field-split: an empty IFS is
   exactly that, so the ordinary assign_words path handles it with no second
   code path to keep in step. */
int	builtin_read(t_shell *state, t_vec argv)
{
	char	*line;
	t_rdopt	o;
	int		eof;

	o = (t_rdopt){.nchars = -1, .delim = '\n', .tmo_ms = -1};
	o.first = parse_read_opts2(argv, &o);
	o.ifs = dup_ifs(state);
	if (o.exact)
		o.ifs = (xfree(o.ifs), ft_strdup(""));
	if (o.prompt && isatty(STDIN_FILENO))
		if (write(2, o.prompt, ft_strlen(o.prompt)) < 0)
			o.prompt = NULL;
	if (rd_wait_input(&o))
		return (xfree(o.ifs), 142);
	line = read_one_line(&o, &eof);
	if (!line)
		return (xfree(o.ifs), 1);
	rd_dispatch(state, argv, line, &o);
	return (xfree(o.ifs), eof != 0);
}
