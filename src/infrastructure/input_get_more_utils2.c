/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_get_more_utils2.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 17:33:40 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:14:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

/* After one debug-parser round: save the command to history, compact the ring
   buffer, zero the input accumulator, and refresh the prompt. Same sequence as
   the production path, just separated so the debug loop can call it explicitly
   after printing the AST. */
void	debug_parser_cleanup(t_shell *state, t_deque_tok *tt, char **prompt)
{
	if (tt->deqtok.buff)
		deque_clear(&tt->deqtok, NULL);
	tt->looking_for = 0;
	manage_history(state);
	buff_readline_reset(&state->rl);
	buff_readline_update(&state->rl);
	state->input.len = 0;
	reset_status_and_prompt(state, prompt);
}

/* Print the full token list to stdout, reset exit status, and clear the deque.
   The debug-lexer mode shows this output instead of parsing — handy to check
   that the lexer is splitting words and operators correctly. */
void	debug_lexer_print_tokens(t_shell *state, t_deque_tok *tt)
{
	print_tokens(tt);
	set_cmd_status(state, res_status(0));
	deque_clear(&tt->deqtok, NULL);
	tt->looking_for = 0;
}

/* Companion to debug_lexer_print_tokens: save history, reset the ring buffer
   and input accumulator, and rebuild the prompt for the next round. */
void	debug_lexer_cleanup(t_shell *state, t_deque_tok *tt, char **prompt)
{
	(void)tt;
	manage_history(state);
	buff_readline_reset(&state->rl);
	buff_readline_update(&state->rl);
	state->input.len = 0;
	reset_status_and_prompt(state, prompt);
}
