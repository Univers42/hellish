/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:07 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:14:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"

void	manage_history(t_shell *state)
{
	char	*hist_entry;
	char	*enc_hist_entry;

	if (worthy_of_being_remembered(state))
	{
		if (state->rl.cursor > 0 && state->rl.buff.ctx)
			((char *)state->rl.buff.ctx)
			[state->rl.cursor - 1] = '\0';
		hist_entry = ft_strndup((char *)state->rl.buff.ctx,
				state->rl.cursor - 1);
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
	buff_readline_reset(&state->rl);
}

bool	worthy_of_being_remembered(t_shell *state)
{
	if (state->rl.cursor > 1 && state->hist.hist_active
		&& (!state->hist.hist_cmds.len
			|| !str_slice_eq_str((char *)state->rl.buff.ctx,
				state->rl.cursor - 1,
				((char **)state->hist.hist_cmds.ctx)
				[state->hist.hist_cmds.len - 1]
			)
		)
	)
		return (true);
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

	i = -1;
	while (++i < state->hist.hist_cmds.len)
		free(((char **)state->hist.hist_cmds.ctx)[i]);
	free(state->hist.hist_cmds.ctx);
	vec_init(&state->hist.hist_cmds);
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
