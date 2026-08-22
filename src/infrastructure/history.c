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

/* Feed a command to readline's history.

   Default (bash cmdhist): hist_join_line rewrites command-boundary
   newlines as "; " -- or a space where a ";" would be a syntax error,
   which is not a nicety: `f()` followed by ";" stops defining a function.
   With `shopt -s lithist` the boundaries stay newlines instead.

   Both modes run the SAME scanner, so both keep quoted and here-doc
   newlines literal and both splice out a top-level \<newline>, exactly as
   bash does -- lithist means "keep the layout", not "store the raw bytes".
   hist_cmds and the history file keep the raw text either way. */
void	add_history_line(t_shell *state, const char *cmd)
{
	char	*joined;

	joined = hist_join_line(cmd, (state->shopt & SHOPT_LITHIST) != 0);
	if (!joined)
		return (add_history(cmd));
	add_history(joined);
	xfree(joined);
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
