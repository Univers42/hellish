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

void	default_parser_loop(t_shell *state, t_parser *parser,
								char **prompt, t_deque_tok *tt)
{
	int	s;

	while (parser->res == RES_GETMOREINPUT || parser->res == RES_INIT)
	{
		s = get_more_tokens(state, prompt, tt);
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
