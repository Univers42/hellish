/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:12 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:29:05 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

/* Initialise the parser, token deque, and prompt string for one command cycle.
   For an interactive session (INP_RL) we build the coloured prompt via
   prompt_normal(); for scripts/pipes we use an empty string (no prompt needed).
*/
static void	prepare_parser_and_prompt(t_shell *state,
									t_parser *parser,
									t_deque_tok *tt,
									char **prompt)
{
	t_string	p;

	*parser = (t_parser){.res = RES_INIT};
	vec_init(&parser->parse_stack);
	parser->parse_stack.elem_size = sizeof(int);
	if (state->metinp == INP_RL)
	{
		p = prompt_normal(state);
		*prompt = ft_strdup(p.ctx);
		xfree(p.ctx);
	}
	else
		*prompt = ft_strdup("");
	*tt = (t_deque_tok){0};
	deque_init(&tt->deqtok, 100, sizeof(t_token));
	tt->looking_for = 0;
}

/* After the parse loop settles: execute the tree if parsing succeeded, handle
   any lingering signal cancellation, save history, and free all scratch
   allocations. should_exit is also updated here: a non-interactive interrupt
   or an exhausted input stream ends the shell. */
static void	finalize_parser_and_cleanup(t_shell *state,
										t_parser *parser,
										t_deque_tok *tt,
										char *prompt)
{
	if (parser->res == RES_OK)
	{
		execute_top_level(state);
		free_ast(&state->tree);
	}
	if (get_g_sig()->should_unwind)
		set_cmd_status(state, create_exec_state(CANCELED, true));
	manage_history(state);
	if (parser->parse_stack.ctx)
		xfree(parser->parse_stack.ctx);
	parser->parse_stack = (t_vec_int){};
	if (prompt)
		xfree(prompt);
	if (tt->deqtok.buff)
		xfree(tt->deqtok.buff);
	state->should_exit |= (get_g_sig()->should_unwind
			&& state->metinp != INP_RL)
		|| state->rl.has_finished;
}

/* One full command cycle: build the prompt, read+lex+parse until we have a
   complete command (or hit EOF/error), execute it, then clean up. This is the
   inner body of the shell's REPL; shell.c calls it in a loop until should_exit
   is set. All memory allocated during a cycle is freed before returning, so
   even a long-running script stays leak-flat (the pile of xfree at the bottom
   of finalize_parser_and_cleanup is the whole trick). */
void	parse_and_execute_input(t_shell *state)
{
	t_deque_tok		tt;
	char			*prompt;
	t_parser		parser;

	prepare_parser_and_prompt(state, &parser, &tt, &prompt);
	get_more_input_parser(state, &parser, &prompt, &tt);
	finalize_parser_and_cleanup(state, &parser, &tt, prompt);
}
