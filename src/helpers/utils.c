/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:35:00 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:35:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "env.h"
#include "libft.h"
#include <stdarg.h>

/* write() can short-write when the buffer is full or a signal interrupts it.
   This wrapper retries until every byte is out or a real error happens.
   Returns 0 on success, 1 on error. NULL or empty string are a fast-path
   no-op, avoiding a write(fd, "", 0) that some kernels count as a flush. */
int	write_to_file(char *str, int fd)
{
	int	wrote_total;
	int	wrote;
	int	len;

	if (!str)
		return (1);
	len = ft_strlen(str);
	if (len == 0)
		return (0);
	wrote_total = 0;
	while (wrote_total != len)
	{
		wrote = write(fd, str + wrote_total, len - wrote_total);
		if (wrote < 0)
			return (1);
		wrote_total += wrote;
	}
	return (0);
}

/* Forward the shell's exit status to the OS. If the last command was killed
   by Ctrl-C we restore the default SIGINT handler first, then exit(128+2) --
   that lets the parent shell see a signal-killed exit code rather than 130
   from our own handler. raise() was tried here but is pointless: we already
   have the right exit code, and raising just before exit() adds noise with
   no benefit. ft_assert(status != -1) is a guard against us forgetting to
   set the state before calling this; -1 is the sentinel "uninitialised". */
void	forward_exit_status(t_execution_state res)
{
	ft_assert(res.status != -1);
	if (res.ctrl_c)
	{
		default_signal_handlers();
		exit(128 + SIGINT);
	}
	exit(res.status);
}

/* Decimal-format `n` into `buf` (no allocation). Negative statuses cannot
   normally happen, but the formatter stays int-safe anyway (long arithmetic
   dodges the -INT_MIN overflow). Digits are built in reverse in tmp then
   flipped into buf. */
static void	fmt_status(char *buf, long n)
{
	char	tmp[12];
	int		i;
	int		j;

	i = 0;
	j = 0;
	if (n < 0)
	{
		buf[j++] = '-';
		n = -n;
	}
	if (n == 0)
		tmp[i++] = '0';
	while (n > 0)
	{
		tmp[i++] = (char)('0' + (n % 10));
		n /= 10;
	}
	while (i > 0)
		buf[j++] = tmp[--i];
	buf[j] = '\0';
}

/* Record the result of the last command in both forms the shell needs: the
   numeric t_execution_state for logic (pipefail, ctrl_c detection) and the
   pre-formatted string for $? expansion.  The string is formatted into the
   statbuf scratch inside t_shell (same pattern as flagbuf/linebuf): $? is
   re-set after every command, so the previous ft_itoa + xfree pair was one
   malloc/free per command for a string of at most 4 bytes. last_cmd_st
   points into the struct and is never freed. */
void	set_cmd_status(t_shell *state, t_execution_state res)
{
	state->last_cmd_st_exe = res;
	fmt_status(state->statbuf, res.status);
	state->last_cmd_st = state->statbuf;
}
