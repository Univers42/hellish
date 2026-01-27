/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   buffered_readline.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:04 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

int	return_last_line(t_shell *state, t_string *ret)
{
	int	len;

	len = state->readline_buff.buff.len - state->readline_buff.cursor;
	vec_push_nstr(ret, (char *)state->readline_buff.buff.ctx
		+ state->readline_buff.cursor, len);
	state->readline_buff.cursor = 0;
	state->readline_buff.buff.len = 0;
	state->readline_buff.has_line = false;
	if (len == 0)
		return (1);
	return (4);
}

int	return_new_line(t_shell *state, t_string *ret)
{
	char	*temp;
	int		len;

	state->readline_buff.line++;
	update_context(state);
	temp = ft_strchr((char *)state->readline_buff.buff.ctx
			+ state->readline_buff.cursor, '\n');
	if (temp == 0)
		return (return_last_line(state, ret));
	len = temp - ((char *)state->readline_buff.buff.ctx
			+ state->readline_buff.cursor) + 1;
	if (len)
		vec_push_nstr(ret, (char *)state->readline_buff.buff.ctx
			+ state->readline_buff.cursor, len);
	state->readline_buff.cursor += len;
	state->readline_buff.has_line = state->readline_buff.cursor
		!= state->readline_buff.buff.len;
	if (len == 0)
		return (1);
	return (4);
}

int	buff_readline(t_shell *state, t_string *ret, char *prompt)
{
	int	code;

	if (state->readline_buff.has_finished)
		return (0);
	if (!state->readline_buff.has_line)
	{
		if (state->input_method == INP_ARG || state->input_method == INP_FILE)
			return (state->readline_buff.has_finished = true, 0);
		if (state->input_method == INP_NOTTY)
			code = get_more_input_notty(state);
		else
			code = get_more_input_readline(&state->readline_buff, prompt);
		if (code == 1)
			return (state->readline_buff.has_finished = true, 0);
		if (code == 2)
		{
			get_g_sig()->should_unwind = SIGINT;
			set_cmd_status(state, (t_execution_state){.status = CANCELED, .c_c = true});
			return (2);
		}
		if (state->input_method == INP_RL)
			vec_push_char(&state->readline_buff.buff, '\n');
		state->readline_buff.has_line = true;
	}
	return (return_new_line(state, ret));
}
