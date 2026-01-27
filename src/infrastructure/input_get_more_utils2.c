/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_get_more_utils2.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 17:33:40 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:07:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

void	debug_parser_cleanup(t_shell *state, t_deque_tok *tt, char **prompt)
{
	if (tt->deqtok.buff)
		deque_clear(&tt->deqtok, NULL);
	tt->looking_for = 0;
	manage_history(state);
	buff_readline_reset(&state->readline_buff);
	buff_readline_update(&state->readline_buff);
	state->input.len = 0;
	reset_status_and_prompt(state, prompt);
}

void	debug_lexer_print_tokens(t_shell *state, t_deque_tok *tt)
{
	print_tokens(tt);
	set_cmd_status(state, res_status(0));
	deque_clear(&tt->deqtok, NULL);
	tt->looking_for = 0;
}

void	debug_lexer_cleanup(t_shell *state, t_deque_tok *tt, char **prompt)
{
	(void)tt;
	manage_history(state);
	buff_readline_reset(&state->readline_buff);
	buff_readline_update(&state->readline_buff);
	state->input.len = 0;
	reset_status_and_prompt(state, prompt);
}
