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

#include "zle.h"
#include "rl_private.h"

/* Drain everything left in the buffer into ret and declare the buffer empty.
   Returns 1 (EOF-like) if there was nothing, 4 (data) if bytes were moved.
   The 1/4 distinction lets the caller tell "empty line" from "no data at all".
*/
int	return_last_line(t_shell *state, t_string *ret)
{
	int	len;

	len = state->rl.buff.len - state->rl.cursor;
	vec_push_nstr(ret, (char *)state->rl.buff.ctx
		+ state->rl.cursor, len);
	state->rl.cursor = 0;
	state->rl.buff.len = 0;
	state->rl.has_line = false;
	if (len == 0)
		return (1);
	return (4);
}

/* Deliver one logical line (up to and including '\n') from the buffer into
   ret, advancing the cursor past it. If there is no '\n' the whole remainder
   is handed off via return_last_line. Also increments the line counter (for
   "script: line N:" error messages) and refreshes the context string. */
int	return_new_line(t_shell *state, t_string *ret)
{
	char	*temp;
	int		len;

	state->rl.line++;
	update_ctx(state);
	temp = ft_strchr((char *)state->rl.buff.ctx
			+ state->rl.cursor, '\n');
	if (temp == 0)
		return (return_last_line(state, ret));
	len = temp - ((char *)state->rl.buff.ctx
			+ state->rl.cursor) + 1;
	if (len)
		vec_push_nstr(ret, (char *)state->rl.buff.ctx
			+ state->rl.cursor, len);
	state->rl.cursor += len;
	state->rl.has_line = state->rl.cursor
		!= state->rl.buff.len;
	if (len == 0)
		return (1);
	return (4);
}

/* Refill the ring buffer from the input source. Returns 0 when data is
   ready, 1 on EOF (INP_ARG/INP_FILE are fully preloaded, so an empty ring
   IS end-of-input for them), 2 on SIGINT during the read. */
static int	fetch_more(t_shell *state, char *prompt)
{
	int	code;

	if (state->metinp == INP_ARG || state->metinp == INP_FILE)
		return (1);
	if (state->metinp == INP_NOTTY)
		code = get_more_input_notty(state);
	else
	{
		*zle_caller_cell() = state;
		code = get_more_input_readline(&state->rl, prompt);
	}
	if (code == 1 || code == 2)
		return (code);
	if (state->metinp == INP_RL)
		vec_push_char(&state->rl.buff, '\n');
	state->rl.has_line = true;
	return (0);
}

/* The central dispatcher: pull input into ret, fetching more raw bytes
   first if the internal buffer is exhausted. Return codes:
     0 = EOF / hard exit (caller should set has_finished)
     2 = interrupted by SIGINT (propagate cancel)
     4 = data was delivered
   Non-interactive sources hand over every complete hazard-free line in one
   batch (return_batch); the single-line path remains for the REPL and for
   input the batch scanner refuses (aliases, heredocs, continuations). */
int	buff_readline(t_shell *state, t_string *ret, char *prompt)
{
	int	code;

	if (state->rl.has_finished)
		return (0);
	begin_cycle(state, ret);
	if (!state->rl.has_line)
	{
		code = fetch_more(state, prompt);
		if (code == 1)
			return (state->rl.has_finished = true, 0);
		if (code == 2)
		{
			get_g_sig()->should_unwind = SIGINT;
			set_cmd_status(state, create_exec_state(CANCELED, true));
			return (2);
		}
	}
	if (state->metinp != INP_RL && return_batch(state, ret))
		return (4);
	return (return_new_line(state, ret));
}
