/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_outstream.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define _GNU_SOURCE
#include "rl_private.h"

/* readline's output stream: the terminal on fd 2, as bash has it -- but
** BUFFERED, and written through tty_write_all.
**
** It was stderr, which is unbuffered, and readline 8.2 draws the prompt
** with putc: the row it owns reached the tty one byte per write() -- 26 of
** them for `\e[38;2;152;195;121m> \e[0m`. Every write() is a place the
** line discipline puts out an echo it still owes (n_tty_write runs
** process_echoes first, whatever ECHO is by then): a key typed before
** readline took raw mode, while the terminal's reader was behind, came
** out between two of those bytes -- inside the colour escape, whose tail
** then printed as text. That is the split prompt_atomic_test.py found on a
** loaded CI runner in the one row rl_prerow.c could not account for.
**
** bash line-buffers stderr for the whole shell (shell_initialize). Here
** only readline's stream is buffered: readline flushes it at the end of
** every redisplay and around raw mode, so a row goes out in one write(),
** and prompt_row_writes_test.py pins that no escape or glyph is ever cut
** by a write() boundary. The flush goes through tty_write_all, like every
** other prompt writer since #34, so a terminal some program left
** non-blocking loses nothing on EAGAIN. It writes to fd 2 as it is at the
** time, so `exec 2>file` still takes the prompt with it. The stream has
** no descriptor of its own: what needs one uses STDERR_FILENO. */

#ifdef __APPLE__

static int	rl_out_cookie(void *cookie, const char *buf, int len)
{
	(void)cookie;
	tty_write_all(STDERR_FILENO, buf, (size_t)len);
	return (len);
}

static FILE	*rl_out_make(void)
{
	return (funopen(NULL, NULL, rl_out_cookie, NULL, NULL));
}

#else

static ssize_t	rl_out_cookie(void *cookie, const char *buf, size_t len)
{
	(void)cookie;
	tty_write_all(STDERR_FILENO, buf, len);
	return ((ssize_t)len);
}

static FILE	*rl_out_make(void)
{
	cookie_io_functions_t	io;

	ft_memset(&io, 0, sizeof(io));
	io.write = rl_out_cookie;
	return (fopencookie(NULL, "w", io));
}

#endif

/* Made once, at readline's first setup. stderr if the stream cannot be
   made -- the old behaviour, not a failure. */
FILE	*rl_out_open(void)
{
	static char	buf[16384];
	FILE		*f;

	f = rl_out_make();
	if (!f)
		return (stderr);
	setvbuf(f, buf, _IOFBF, sizeof(buf));
	return (f);
}
