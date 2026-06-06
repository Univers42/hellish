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

/* Scan leading option words (anything starting with '-' and having at least
   one more character). Only -r is meaningful: set *raw so read_one_line and
   next_field skip backslash processing. Other letters are silently ignored —
   we parse them rather than stopping so `-rp "prompt: "` does not treat `-p`
   as the first variable name. Returns the index of the first non-option arg
   (i.e., the first variable name). */
size_t	parse_read_opts(t_vec argv, bool *raw)
{
	size_t	i;
	int		j;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		j = 1;
		while (((char **)argv.ctx)[i][j])
		{
			if (((char **)argv.ctx)[i][j] == 'r')
				*raw = true;
			j++;
		}
		i++;
	}
	return (i);
}

/* read [-r] [var ...]: read one line from stdin and split it into variables.
   If no variable names are given, the whole line goes into $REPLY (POSIX).
   Returns 1 on EOF even if some data was read (mimics bash), so `while read
   line; do …; done` processes the last line before stopping even when the
   file lacks a trailing newline. */
int	builtin_read(t_shell *state, t_vec argv)
{
	char	*line;
	t_rdopt	o;
	int		eof;

	o.raw = false;
	o.first = parse_read_opts(argv, &o.raw);
	o.ifs = dup_ifs(state);
	line = read_one_line(o.raw, &eof);
	if (!line)
		return (xfree(o.ifs), 1);
	if (o.first >= argv.len)
		rd_set_var(state, "REPLY", line);
	else
	{
		assign_words(state, line, argv, &o);
		xfree(line);
	}
	return (xfree(o.ifs), eof != 0);
}
