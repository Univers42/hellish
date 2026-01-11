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

#include "shell.h"
#include <fcntl.h>
#include <stdio.h>
#include <readline/history.h>
#include <stdbool.h>
#include <unistd.h>
# include "helpers.h"
# include "env.h"

t_string	parse_single_cmd(t_string hist, size_t *cur)
{
	t_string	cmd;
	bool		bs;
	char		c;

	vec_init(&cmd);
	cmd.elem_size = 1;
	bs = false;
	while (*cur < hist.len)
	{
		c = ((char *)hist.ctx)[*cur];
		if (c == '\\' && !bs)
		{
			bs = true;
			(*cur)++;
			continue ;
		}
		if (c == '\n' && !bs)
		{
			(*cur)++;
			break ;
		}
		vec_push(&cmd, &c);
		bs = false;
		(*cur)++;
	}
	// Ensure null-termination for add_history
	if (!vec_ensure_space_n(&cmd, 1)) {
		// handle allocation error if needed
	}
	((char *)cmd.ctx)[cmd.len] = '\0';
	return (cmd);
}

t_vec	parse_hist_file(t_string hist)
{
	size_t		cur;
	t_vec	ret;
	char		*cmd;

	cur = 0;
	vec_init(&ret);
	ret.elem_size = sizeof(char *); // <--- store pointers
	while (cur < hist.len)
	{
		cmd = (char *)parse_single_cmd(hist, &cur).ctx;
		vec_push(&ret, &cmd); // <--- push address of pointer
		add_history(cmd);
	}
	return (ret);
}

void	parse_history_file(t_shell *state)
{
	t_string	hist;
	int			fd;
	char		*hist_file_path;

	hist_file_path = get_hist_file_path(state);
	if (!hist_file_path)
		return ;
	fd = open(hist_file_path, O_RDONLY | O_CREAT, 0666);
	if (fd < 0)
	{
		warning_error("Can't open the history file for reading");
		free(hist_file_path);
		return ;
	}
	vec_init(&hist);
	hist.elem_size = 1;
	vec_append_fd(fd, &hist);
	close(fd);
	state->hist.hist_cmds = parse_hist_file(hist);
	state->hist.append_fd
		= open(hist_file_path, O_CREAT | O_WRONLY | O_APPEND, 0666);
	if (state->hist.append_fd < 0)
		warning_error("Can't open the history file for writing");
	free(hist_file_path);
	free(hist.ctx);
}

t_string	encode_cmd_hist(char *cmd)
{
	t_string	ret;

	vec_init(&ret);
	ret.elem_size = 1;
	while (*cmd)
	{
		if (*cmd == '\\') {
			char tmp = '\\'; vec_push(&ret, &tmp);
		}
		if (*cmd == '\n') {
			char tmp = '\\'; vec_push(&ret, &tmp);
		}
		{ char tmp = *cmd; vec_push(&ret, &tmp); }
		cmd++;
	}
	{ char tmp = '\n'; vec_push(&ret, &tmp); }
	// Ensure null-termination for write_to_file
	if (!vec_ensure_space_n(&ret, 1)) {
		// handle allocation error if needed
	}
	((char *)ret.ctx)[ret.len] = '\0';
	return (ret);
}

void	manage_history(t_shell *state)
{
	char	*hist_entry;
	char	*enc_hist_entry;

	if (worthy_of_being_remembered(state))
	{
		// Ensure null-termination before using ft_strndup
		if (state->readline_buff.cursor > 0 && state->readline_buff.buff.ctx)
			((char *)state->readline_buff.buff.ctx)[state->readline_buff.cursor - 1] = '\0';
		hist_entry = ft_strndup((char *)state->readline_buff.buff.ctx,
				state->readline_buff.cursor - 1);
		add_history(hist_entry);
		vec_push(&state->hist.hist_cmds, &hist_entry);
		if (state->hist.append_fd >= 0)
		{
			enc_hist_entry = (char *)encode_cmd_hist(hist_entry).ctx;
			if (write_to_file(enc_hist_entry, state->hist.append_fd))
			{
				warning_error("Failed to write to the history file");
				close(state->hist.append_fd);
				state->hist.append_fd = -1;
			}
			free(enc_hist_entry);
		}
	}
	buff_readline_reset(&state->readline_buff);
}

bool	worthy_of_being_remembered(t_shell *state)
{
	if (state->readline_buff.cursor > 1 && state->hist.hist_active
		&& (!state->hist.hist_cmds.len
			|| !str_slice_eq_str((char *)state->readline_buff.buff.ctx,
				state->readline_buff.cursor - 1,
				((char **)state->hist.hist_cmds.ctx)[state->hist.hist_cmds.len - 1]
			)
		)
	)
	{
		return (true);
	}
	return (false);
}

void	init_history(t_shell *state)
{
	state->hist = (t_history){.append_fd = -1, .hist_active = true};
	parse_history_file(state);
}

void	free_hist(t_shell *state)
{
	size_t	i;

	i = 0;
	while (i < state->hist.hist_cmds.len)
	{
		free(((char **)state->hist.hist_cmds.ctx)[i]);
		i++;
	}
	free(state->hist.hist_cmds.ctx);
	vec_init(&state->hist.hist_cmds);
}

char	*get_hist_file_path(t_shell *state)
{
	t_env		*env;
	t_string	full_path;

	env = env_get(&state->env, "HOME");
	if (!env || !env->value)
	{
		warning_error("HOME is not set, can't get the history");
		return (0);
	}
	vec_init(&full_path);
	full_path.elem_size = 1;
	vec_push_str(&full_path, env->value);
	if (!vec_str_ends_with_str(&full_path, "/")) {
		char tmp = '/'; vec_push(&full_path, &tmp);
	}
	vec_push_str(&full_path, HIST_FILE);
	return (char *)full_path.ctx;
}

