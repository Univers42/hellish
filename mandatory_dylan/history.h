/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 20:10:20 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 20:10:20 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HISTORY_H
# define HISTORY_H

# include "common.h"
# include "env.h"

# define HIST_FILE ".minishell_history"

typedef struct s_hist
{
	bool	active;
	int		append_fd;
	t_vec	hist_cmds;
}	t_hist;

static inline t_string	parse_single_cmd(t_string hist, size_t *cur)
{
	t_string	cmd;
	bool bs;
	char c;

	vec_init(&cmd);
	bs = false;
	while (*cur < hist.len)
	{
		c = hist.buff[*cur];
		if (c == '\\' && !bs)
		{
			bs = true;
			(*cur)++;
			continue;
		}
		if (c == '\n' && !bs)
		{
			(*cur)++;
			break;
		}
		vec_push(&cmd, c);
		bs = false;
		(*cur)++;
	}
	return (cmd);
}

static inline t_strings	parse_hist_file(t_string hist)
{
	size_t		cur;
	t_strings	ret;
	char		*cmd;
	char		*cmd_ptr;

	cur = 0;
	vec_init(&ret, );
	while (cur < hist.len)
	{
		cmd = parse_single_cmd(hist, &cur).buff;
		cmd_ptr = cmd;
		vec_push(&ret, &cmd_ptr);
		add_history(cmd);
	}
	return (ret);
}

static inline char	*get_hist_file_path(t_envs *_env)
{
	t_env		*env;
	t_string	full_path;

	env = env_get(_env, "HOME");
	if (!env || !env->value)
	{
		warning_error("HOME is not set, can't get the history");
		return (0);
	}
	vec_init(&full_path);
	vec_push(&full_path, env->value);
	if (!vec_append(&full_path, "/"))
		vec_push(&full_path, '/');
	vec_push(&full_path, HIST_FILE);
	return (full_path.buff);
}

static inline void	parse_history_file(t_hist *_hist, t_envs *env)
{
	t_string	hist;
	int			fd;
	char		*hist_file_path;

	hist_file_path = get_hist_file_path(env);
	if (!hist_file_path)
		return ;
	fd = open(hist_file_path, O_RDONLY | O_CREAT, 0666);
	if (fd < 0)
	{
		warning_error("Can't open the history file for reading");
		free(hist_file_path);
		return;
	}
	vec_init(&hist);
	vec_append_fd(&hist, fd);
	close(fd);
	_hist->cmds = parse_hist_file(hist);
	_hist->append_fd = open(hist_file_path, O_CREAT | O_WRONLY | O_APPEND, 0666);
	if (_hist->append_fd < 0)
		warning_error("Can't open the history file for writing");
	free(hist_file_path);
	free(hist.buff);
}

static inline t_string encode_cmd_hist(char *cmd)
{
	t_string ret;

	vec_init(&ret);
	while (*cmd)
	{
		if (*cmd == '\\')
			vec_push(&ret, '\\');
		if (*cmd == '\n')
			vec_push(&ret, '\\');
		vec_push(&ret, *cmd);
		cmd++;
	}
	vec_push(&ret, '\n');
	return (ret);
}

static inline bool worthy_of_being_remembered(t_hist *hist, t_rl *rl)
{
	// Determine actual entered command length:
	// - prefer rl->cursor (cursor points after last typed char),
	// - fallback to entire buffer length if cursor is not set.
	size_t entry_len = 0;
	if (rl->cursor > 0)
	{
		// rl->cursor is count of characters including a trailing newline pushed by xreadline
		// historically callers used rl->cursor - 1; guard against underflow
		if (rl->cursor > 0)
			entry_len = rl->cursor - 1;
	}
	if (entry_len == 0)
		entry_len = rl->str.len;

	// Only remember non-empty entries and when history is active.
	if (entry_len > 0 && hist->active &&
		( !hist->cmds.len ||
		  !str_slice_eq_str(rl->str.buff, entry_len,
							*(char **)vec_idx(&hist->cmds, hist->cmds.len - 1)) ))
	{
		return (true);
	}
	return (false);
}

static inline void manage_history(t_hist *hist, t_rl *rl)
{
	char *hist_entry;
	char *enc_hist_entry;

	/* compute entry length the same way as worthy_of_being_remembered */
	size_t entry_len = 0;
	if (rl->cursor > 0)
	{
		/* guard: rl->cursor is count including trailing newline pushed by xreadline */
		if (rl->cursor > 0)
			entry_len = rl->cursor - 1;
	}
	if (entry_len == 0)
		entry_len = rl->str.len;

	if (entry_len == 0)
		return;

	if (worthy_of_being_remembered(hist, rl))
	{
		hist_entry = ft_strndup(rl->str.buff, entry_len);
		add_history(hist_entry);
		vec_push(&hist->cmds, &hist_entry);
		if (hist->append_fd >= 0)
		{
			enc_hist_entry = encode_cmd_hist(hist_entry).buff;
			if (write_to_file(enc_hist_entry, hist->append_fd) < 0)
			{
				warning_error("Failed to write to the history file");
				close(hist->append_fd);
				hist->append_fd = -1;
			}
			free(enc_hist_entry);
		}
	}
}

static inline void manage_history_input(t_hist *hist, t_rl *rl, t_dyn_str *input)
{
	char *hist_entry;
	char *enc_hist_entry;
	size_t entry_len;

	entry_len = input->len;
	/* trim trailing newline if present */
	if (entry_len > 0 && input->buff[entry_len - 1] == '\n')
		entry_len--;
	if (entry_len == 0)
		return ;
	/* skip duplicate of last history entry */
	if (hist->cmds.len)
	{
		char *last = *(char **)vec_idx(&hist->cmds, hist->cmds.len - 1);
		if (last && str_slice_eq_str(input->buff, entry_len, last))
			return;
	}
	hist_entry = ft_strndup(input->buff, entry_len);
	add_history(hist_entry);
	vec_push(&hist->cmds, &hist_entry);
	if (hist->append_fd >= 0)
	{
		enc_hist_entry = encode_cmd_hist(hist_entry).buff;
		if (write_to_file(enc_hist_entry, hist->append_fd) < 0)
		{
			warning_error("Failed to write to the history file");
			close(hist->append_fd);
			hist->append_fd = -1;
		}
		free(enc_hist_entry);
	}
}

static inline void init_history(t_hist *hist, t_vec *env)
{
	*hist = (t_hist){.append_fd = -1, .active = true};
	parse_history_file(hist, env);
}

static inline void free_hist(t_hist *hist)
{
	size_t i;

	i = 0;
	while (i < hist->cmds.len)
	{
		free(*(char **)vec_idx(&hist->cmds, i));
		i++;
	}
	free(hist->cmds.buff);
	vec_init(&hist->cmds, &(t_vec_config){0});
}
# endif