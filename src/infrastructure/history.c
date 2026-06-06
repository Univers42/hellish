/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:41 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:41 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include "libft.h"

/* Walk one raw history entry (from the file) into cmd, unescaping the encoding
   used by encode_cmd_hist. A backslash before a newline is the escape for a
   real newline in the original command (a \-continuation or a here-string
   body). A lone \n terminates the entry. The bool *bs tracks whether we just
   consumed a backslash so the pair is processed atomically. */
static void	process_parse_single_cmd(t_string hist, size_t *cur,
				t_string *cmd, bool *bs)
{
	char	c;

	while (*cur < hist.len)
	{
		c = ((char *)hist.ctx)[*cur];
		if (c == '\\' && !(*bs))
		{
			*bs = true;
			(*cur)++;
			continue ;
		}
		if (c == '\n' && !(*bs))
		{
			(*cur)++;
			break ;
		}
		vec_push(cmd, &c);
		*bs = false;
		(*cur)++;
	}
}

/* Public wrapper: allocate and return one decoded entry from the history
   buffer. The returned t_string's .ctx is NUL-terminated for safe use as a
   C string but .len does not include the terminator. */
t_string	parse_single_cmd(t_string hist, size_t *cur)
{
	t_string	cmd;
	bool		bs;

	vec_init(&cmd);
	cmd.elem_size = 1;
	bs = false;
	process_parse_single_cmd(hist, cur, &cmd, &bs);
	vec_ensure_space_n(&cmd, 1);
	((char *)cmd.ctx)[cmd.len] = '\0';
	return (cmd);
}

/* readline edits each history entry as one logical line, so an entry with an
   embedded newline (a \-continuation or quoted-newline command) desyncs its
   cursor model and corrupts the display on ↑/↓ (you get stuck, needing ^C).
   Feed readline a single-line form: drop a \ that escapes a newline (join the
   continuation) and turn any other newline into a space. */
static void	fill_hist_buf(const char *cmd, char *buf)
{
	size_t	i;
	size_t	j;

	i = 0;
	j = 0;
	while (cmd[i])
	{
		if (cmd[i] == '\\' && cmd[i + 1] == '\n')
		{
			i += 2;
			buf[j++] = ' ';
		}
		else if (cmd[i] == '\n')
		{
			i++;
			buf[j++] = ' ';
		}
		else
			buf[j++] = cmd[i++];
	}
	buf[j] = '\0';
}

/* Feed a command string to readline's history, pre-processing it into a single
   logical line: a \<newline> pair (line continuation) becomes a space, and a
   bare \n (embedded newline in a multi-line command) also becomes a space.
   Without this, readline's history movement gets confused by embedded newlines
   and can lock up or display garbled lines on ↑. */
void	add_history_line(const char *cmd)
{
	char	*buf;

	buf = xmalloc(ft_strlen(cmd) + 1);
	if (!buf)
		return (add_history(cmd));
	fill_hist_buf(cmd, buf);
	add_history(buf);
	xfree(buf);
}

/* Decode the entire history file buffer into a vector of heap-allocated C
   strings, one per command. The caller owns each string and is responsible for
   freeing them (see free_hist). */
t_vec	parse_hist_file(t_string hist)
{
	size_t	cur;
	t_vec	ret;
	char	*cmd;

	cur = 0;
	vec_init(&ret);
	ret.elem_size = sizeof(char *);
	while (cur < hist.len)
	{
		cmd = (char *)parse_single_cmd(hist, &cur).ctx;
		vec_push(&ret, &cmd);
	}
	return (ret);
}
