/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   progcomp2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 10:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 10:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "progcomp_private.h"
#include <readline/readline.h>

/* Reading the line the user is editing: which command it is, and what the
   word before the cursor was. Both are plain byte scans of rl_line_buffer
   rather than a re-parse -- the line is mid-edit and frequently not
   parseable at all, and bash's own completion answers from the same
   whitespace-separated view (which is why COMP_WORDBREAKS exists). */

void	pc_push(const char *s, int n)
{
	char	*dup;

	dup = ft_strndup((char *)s, (size_t)n);
	if (dup)
		vec_push(pc_cell(), &dup);
}

static bool	pc_blank(char c)
{
	return (c == ' ' || c == '\t');
}

/* Offset of the first word of the command that `start` belongs to. The
   separator set is the one is_cmd_word uses: POSIX XCU 2.9 makes the word
   after ; | & ( { ` or a newline a new command, so `ls | grep <TAB>` must
   look up grep and not ls. */
static size_t	pc_cmd_off(int start)
{
	int	i;

	i = start;
	while (i > 0 && !ft_strchr(";|&(){\n`", rl_line_buffer[i - 1]))
		i--;
	while (rl_line_buffer[i] && pc_blank(rl_line_buffer[i]))
		i++;
	return ((size_t)i);
}

/* The command word for the argument at `start`, or NULL when `start` IS the
   command word -- that case belongs to the PATH completer, and answering it
   from a compspec would complete `git` itself out of git's own subcommand
   list. */
char	*pc_cmd_word(int start)
{
	size_t	off;
	size_t	n;

	if (!rl_line_buffer || start <= 0)
		return (NULL);
	off = pc_cmd_off(start);
	if (off >= (size_t)start)
		return (NULL);
	n = 0;
	while (rl_line_buffer[off + n] && !pc_blank(rl_line_buffer[off + n]))
		n++;
	if (n == 0)
		return (NULL);
	return (ft_strndup(rl_line_buffer + off, n));
}

/* The word immediately before the cursor -- $3 to a -F function, and what
   most completion scripts actually branch on ("was the last thing I saw
   -o?"). Empty string when there is none, never NULL: the function is
   called with three arguments whatever the line looks like. */
char	*pc_prev_word(int start)
{
	int	i;
	int	e;

	if (!rl_line_buffer)
		return (ft_strdup(""));
	i = start;
	while (i > 0 && pc_blank(rl_line_buffer[i - 1]))
		i--;
	e = i;
	while (i > 0 && !pc_blank(rl_line_buffer[i - 1]))
		i--;
	if (e == i)
		return (ft_strdup(""));
	return (ft_strndup(rl_line_buffer + i, (size_t)(e - i)));
}
