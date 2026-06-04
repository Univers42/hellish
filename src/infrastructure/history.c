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

void	add_history_line(const char *cmd)
{
	char	*buf;

	buf = malloc(ft_strlen(cmd) + 1);
	if (!buf)
		return (add_history(cmd));
	fill_hist_buf(cmd, buf);
	add_history(buf);
	free(buf);
}

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
