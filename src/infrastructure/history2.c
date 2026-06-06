/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:41 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:41 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include "libft.h"

/* Keep only the newest HIST_MAX entries (free + drop the oldest), so a
   runaway history file doesn't slow every startup. Returns true if cut. */
static bool	cap_history(t_vec *h)
{
	size_t	drop;
	size_t	i;

	if (h->len <= HIST_MAX)
		return (false);
	drop = h->len - HIST_MAX;
	i = 0;
	while (i < drop)
		xfree(((char **)h->ctx)[i++]);
	ft_memmove(h->ctx, (char **)h->ctx + drop,
		(h->len - drop) * sizeof(char *));
	h->len -= drop;
	return (true);
}

/* Feed the (capped) entries to readline and rewrite the file when trimmed. */
static void	load_and_persist(t_shell *state, const char *path, bool trimmed)
{
	size_t	i;
	int		fd;
	char	*enc;

	i = 0;
	while (i < state->hist.hist_cmds.len)
		add_history_line(((char **)state->hist.hist_cmds.ctx)[i++]);
	if (!trimmed)
		return ;
	fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0600);
	if (fd < 0)
		return ;
	i = 0;
	while (i < state->hist.hist_cmds.len)
	{
		enc = (char *)encode_cmd_hist(
				((char **)state->hist.hist_cmds.ctx)[i++]).ctx;
		if (write(fd, enc, ft_strlen(enc)))
		{
		}
		xfree(enc);
	}
	close(fd);
}

void	parse_history_file(t_shell *state)
{
	t_string	hist;
	int			fd;
	char		*path;
	bool		trimmed;

	path = get_hist_file_path(state);
	if (!path)
		return ;
	fd = open(path, O_RDONLY | O_CREAT, 0666);
	if (fd < 0)
		return (warning_error("Can't open the history file for reading"),
			xfree(path));
	vec_init(&hist);
	hist.elem_size = 1;
	vec_append_fd(fd, &hist);
	close(fd);
	state->hist.hist_cmds = parse_hist_file(hist);
	xfree(hist.ctx);
	trimmed = cap_history(&state->hist.hist_cmds);
	load_and_persist(state, path, trimmed);
	state->hist.append_fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0666);
	if (state->hist.append_fd < 0)
		warning_error("Can't open the history file for writing");
	xfree(path);
}

t_string	encode_cmd_hist(char *cmd)
{
	t_string	ret;

	vec_init(&ret);
	ret.elem_size = 1;
	while (*cmd)
	{
		if (*cmd == '\\')
			vec_push_char(&ret, '\\');
		if (*cmd == '\n')
			vec_push_char(&ret, '\\');
		vec_push_char(&ret, *cmd);
		cmd++;
	}
	vec_push_char(&ret, '\n');
	vec_ensure_space_n(&ret, 1);
	((char *)ret.ctx)[ret.len] = '\0';
	return (ret);
}

char	*get_hist_file_path(t_shell *state)
{
	t_env		*env;
	t_string	full_path;
	char		tmp;

	env = env_get(&state->env, "HOME");
	if (!env || !env->value)
		return (warning_error("HOME is not set, can't get the history"), NULL);
	vec_init(&full_path);
	full_path.elem_size = 1;
	vec_push_str(&full_path, env->value);
	if (!vec_str_ends_with_str(&full_path, "/"))
	{
		tmp = '/';
		vec_push(&full_path, &tmp);
	}
	vec_push_str(&full_path, HIST_FILE);
	return ((char *)full_path.ctx);
}
