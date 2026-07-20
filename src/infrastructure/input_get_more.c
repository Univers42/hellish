/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_get_more.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:10 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:29:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

/* Fetch one round of tokens, with one Ctrl-C retry. The retry matters: when
   a user hits ^C mid-compound-command (e.g. inside a for loop being typed),
   we want to try once more before giving up, in case the first interrupt
   flushed a partial token that left the deque non-empty (return 3 = print
   those orphaned tokens and continue, so debug-lexer mode stays useful). */
static int	fetch_lexer_input(t_shell *state, t_parser *parser,
						char **prompt, t_deque_tok *tt)
{
	int	s;

	s = get_more_tokens(state, prompt, tt);
	(void)parser;
	if (handle_eof(s, state))
		return (1);
	if (handle_interrupt(s, state, tt, prompt) == 2)
	{
		s = get_more_tokens(state, prompt, tt);
		if (handle_eof(s, state))
			return (1);
		if (handle_interrupt(s, state, tt, prompt) == 2)
			return (2);
		if (!is_empty_token_list(tt))
			return (3);
		set_cmd_status(state, res_status(0));
	}
	return (0);
}

/* One iteration of the debug-lexer loop: fetch tokens, check for exit/signal
   conditions, and either print the accumulated token list or reset for the next
   command. "Debug lexer" mode is active with --debug-lexer and prints each
   token set instead of parsing and executing -- handy when chasing lexer bugs.
*/
static int	debug_lexer_loop_body(t_shell *state,
								t_parser *parser,
								char **prompt,
								t_deque_tok *tt)
{
	int	action;

	set_cmd_status(state, res_status(0));
	action = fetch_lexer_input(state, parser, prompt, tt);
	if (action == 1)
		return (1);
	if (action == 2)
		return (2);
	if (action == 3)
	{
		print_and_cleanup_tokens(state, tt, prompt);
		return (0);
	}
	set_cmd_status(state, res_status(0));
	if (get_g_sig()->should_unwind)
		set_cmd_status(state, create_exec_state(CANCELED, true));
	if (state->should_exit || get_g_sig()->should_unwind)
		return (1);
	if (is_empty_token_list(tt))
	{
		buff_readline_reset(&state->rl);
		return (0);
	}
	print_and_cleanup_tokens(state, tt, prompt);
	return (0);
}

/* Drive the debug-lexer REPL: like the normal parser loop but we never parse,
   we just dump the raw token stream. Useful to verify the lexer in isolation
   without parser noise. Activated by the --debug-lexer flag at startup. */
void	debug_lexer_loop(t_shell *state,
							t_parser *parser,
							char **prompt,
							t_deque_tok *tt)
{
	int	ret;

	while (parser->res == RES_GETMOREINPUT || parser->res == RES_INIT)
	{
		ret = debug_lexer_loop_body(state, parser, prompt, tt);
		if (ret == 1)
			break ;
		if (ret == 2)
			continue ;
	}
}

/* The normal production parse loop: keep feeding tokens to the parser until it
   either accepts (RES_OK) or gives a hard error. RES_GETMOREINPUT means the
   parser saw an incomplete construct (e.g. "if" without "fi") and needs more
   lines; we loop back, show the continuation prompt, and append. */
void	default_parser_loop(t_shell *state, t_parser *parser,
								char **prompt, t_deque_tok *tt)
{
	int	s;

	while (parser->res == RES_GETMOREINPUT || parser->res == RES_INIT)
	{
		state->gathering_compound = (parser->res == RES_GETMOREINPUT);
		s = get_more_tokens(state, prompt, tt);
		if (s == 5)
			break ;
		if (handle_eof(s, state))
			break ;
		if (s == 2)
		{
			set_cmd_status(state, create_exec_state(CANCELED, true));
			continue ;
		}
		if (get_g_sig()->should_unwind)
			set_cmd_status(state, create_exec_state(CANCELED, true));
		if (state->should_exit || get_g_sig()->should_unwind)
			break ;
		if (!try_parse_tokens(state, parser, tt, prompt))
			break ;
	}
}

/* Route to the right parse loop based on which debug flags are set. In normal
   operation, only default_parser_loop runs. The debug variants exist so you
   can step back and watch the lexer or parser in isolation during development
   without recompiling. */
void	get_more_input_parser(t_shell *state,
							t_parser *parser,
							char **prompt,
							t_deque_tok *tt)
{
	if (state->option_flags & OPT_FLAG_DEBUG_PARSER)
	{
		debug_parser_loop(state, parser, prompt, tt);
		return ;
	}
	if (state->option_flags & OPT_FLAG_DEBUG_LEXER)
	{
		debug_lexer_loop(state, parser, prompt, tt);
		return ;
	}
	default_parser_loop(state, parser, prompt, tt);
}
