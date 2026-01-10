/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:32:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:32:22 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HISTORY_H
# define HISTORY_H

# include "common.h"
void		manage_history(t_state *state);
void		init_history(t_state *state);
void		free_hist(t_state *state);
void		parse_history_file(t_state *state);
t_string	encode_cmd_hist(char *cmd);

typedef struct s_history
{
	bool		hist_active;
	int			append_fd;
	t_vec	hist_cmds;
}	t_history;


static inline bool	worthy_of_being_remembered(t_state *state)
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

static inline void	init_history(t_state *state)
{
	state->hist = (t_history){.append_fd = -1, .hist_active = true};
	parse_history_file(state);
}

static inline void	free_hist(t_state *state)
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

static inline char	*get_hist_file_path(t_state *state)
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

t_string	parse_single_cmd(t_string hist, size_t *cur);
t_vec	parse_hist_file(t_string hist);
void	parse_history_file(t_state *state);
t_string	encode_cmd_hist(char *cmd);
void	manage_history(t_state *state);


#endif