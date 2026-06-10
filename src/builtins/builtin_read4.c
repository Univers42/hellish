/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read4.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 03:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/10 03:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

/* t_rdbuf (the buffered byte source, declared in builtins_private.h):
   POSIX forces `read` to consume exactly one logical line from fd 0 —
   anything read past the newline is stolen from the next reader. On a pipe
   or tty that means one read(2) PER BYTE (no way to push bytes back). On a
   SEEKABLE fd (file, heredoc temp) we can do what bash does: read a block,
   consume bytes from memory, and lseek the fd back over the unconsumed
   tail when the line ends — turning ~50 syscalls per line into 1-2. */

/* Fetch the next byte. Non-seekable fds keep the strict one-byte read;
   seekable fds refill the block buffer only when it runs dry. Returns the
   read(2) convention: 1 = got a byte, 0 = EOF, <0 = error. */
static ssize_t	rb_next(t_rdbuf *rb, char *ch)
{
	if (!rb->seekable)
		return (read(STDIN_FILENO, ch, 1));
	if (rb->pos >= rb->len)
	{
		rb->len = read(STDIN_FILENO, rb->buf, sizeof(rb->buf));
		rb->pos = 0;
		if (rb->len <= 0)
			return (rb->len);
	}
	*ch = rb->buf[rb->pos++];
	return (1);
}

/* Give back the unconsumed tail of the block buffer: rewind the fd to just
   after the last byte the line actually used, so the next reader (the next
   `read` call, or whatever consumes the rest of the file) sees everything
   we over-read. No-op when the buffer was drained exactly or never used. */
static void	rb_rewind(t_rdbuf *rb)
{
	if (rb->seekable && rb->pos < rb->len)
		lseek(STDIN_FILENO, rb->pos - rb->len, SEEK_CUR);
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

/* One step of the line loop. A newline only reaches here when it follows an
   unescaped backslash in non-raw mode (line continuation): pop the pending
   backslash and resume on the next line. Any other byte is appended, and
   the pending-backslash state is updated so `\<newline>` is spotted next
   iteration. Identical logic to the old byte-at-a-time loop. */
static void	consume_char(t_string *buf, char ch, bool raw, bool *bs)
{
	if (ch == '\n')
	{
		buf->len--;
		*bs = false;
	}
	else
	{
		vec_push(buf, &ch);
		*bs = (!raw && ch == '\\' && !*bs);
	}
}

/* Read one logical line. Returns NULL only at EOF with no data. */
char	*read_one_line(bool raw, int *eof)
{
	char		ch;
	t_string	buf;
	ssize_t		n;
	bool		bs;
	t_rdbuf		rb;

	rb = (t_rdbuf){.len = 0, .pos = 0,
		.seekable = (lseek(STDIN_FILENO, 0, SEEK_CUR) != -1)};
	vec_init(&buf);
	buf.elem_size = 1;
	bs = false;
	n = rb_next(&rb, &ch);
	while (n > 0 && !(ch == '\n' && !(bs && !raw)))
	{
		consume_char(&buf, ch, raw, &bs);
		n = rb_next(&rb, &ch);
	}
	rb_rewind(&rb);
	*eof = (n <= 0);
	return (line_build(&buf, *eof));
}
