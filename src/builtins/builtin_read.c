/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/*   Created: 2026/03/15 02:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 02:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

/* Snapshot IFS into an owned copy so later env_set calls inside the read
   loop cannot pull the rug out from under the field-splitting code. Unset
   IFS defaults to <space><tab><newline> per POSIX 2018 §2.6.5. */
char	*dup_ifs(t_shell *state)
{
	char	*v;

	v = env_expand(state, "IFS");
	if (!v)
		return (ft_strdup(" \t\n"));
	return (ft_strdup(v));
}

/* True if c is in the IFS string (and not NUL — the NUL guard means we
   never count the empty string as a delimiter). */
int	is_ifs(char c, const char *ifs)
{
	return (c && ft_strchr(ifs, c) != NULL);
}

/* IFS whitespace characters get special treatment during field splitting:
   they are collapsed (multiple adjacent whitespace IFS chars count as one
   delimiter), unlike non-whitespace IFS chars which each split exactly. */
int	is_ifs_ws(char c, const char *ifs)
{
	return ((c == ' ' || c == '\t' || c == '\n') && is_ifs(c, ifs));
}

/* Terminate and return the buffer. On EOF with an empty buffer we return
   NULL so the caller can signal "nothing read at all" — the standard exit
   status 1 case. A non-empty partial line (EOF in the middle) is still
   returned; the caller propagates eof but treats the line as valid data. */
static char	*line_build(t_string *buf, int eof_flag)
{
	char	nul;

	if (eof_flag && buf->len == 0)
		return (xfree(buf->ctx), NULL);
	nul = '\0';
	vec_push(buf, &nul);
	return ((char *)buf->ctx);
}

/* Read one logical line. Returns NULL only at EOF with no data. */
char	*read_one_line(bool raw, int *eof)
{
	char		ch;
	t_string	buf;
	ssize_t		n;
	bool		bs;

	vec_init(&buf);
	buf.elem_size = 1;
	bs = false;
	n = read(STDIN_FILENO, &ch, 1);
	while (n > 0 && !(ch == '\n' && !(bs && !raw)))
	{
		if (ch == '\n')
		{
			buf.len--;
			bs = false;
		}
		else
		{
			vec_push(&buf, &ch);
			bs = (!raw && ch == '\\' && !bs);
		}
		n = read(STDIN_FILENO, &ch, 1);
	}
	*eof = (n <= 0);
	return (line_build(&buf, *eof));
}
