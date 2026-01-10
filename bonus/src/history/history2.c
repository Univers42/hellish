/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../shell.h"

#include <readline/history.h>
#include <unistd.h>

bool	worthy_of_being_remembered(t_state *state)
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

void	manage_history(t_state *state)
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

void	init_history(t_state *state)
{
	state->hist = (t_history){.append_fd = -1, .hist_active = true};
	parse_history_file(state);
}

void	free_hist(t_state *state)
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
