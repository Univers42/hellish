/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_get_more.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:10 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 17:59:40 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

static int	fetch_lexer_input(t_shell *state, t_parser *parser,
						char **prompt, t_deque_tt *tt)
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
								t_deque_tt *tt)
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
		set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
	if (state->should_exit || get_g_sig()->should_unwind)
		return (1);
	if (is_empty_token_list(tt))
	{
		buff_readline_reset(&state->readline_buff);
		return (0);
	}
	print_and_cleanup_tokens(state, tt, prompt);
	return (0);
}

void	debug_lexer_loop(t_shell *state,
							t_parser *parser,
							char **prompt,
							t_deque_tt *tt)
{
	int	ret;

	while (parser->res == RES_MoreInput || parser->res == RES_Init)
	{
		ret = debug_lexer_loop_body(state, parser, prompt, tt);
		if (ret == 1)
			break ;
		if (ret == 2)
			continue ;
	}
}

void	default_parser_loop(t_shell *state, t_parser *parser,
								char **prompt, t_deque_tt *tt)
{
	int	s;

	while (parser->res == RES_MoreInput || parser->res == RES_Init)
	{
		s = get_more_tokens(state, prompt, tt);
		if (handle_eof(s, state))
			break ;
		if (s == 2)
		{
			set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
			continue ;
		}
		if (get_g_sig()->should_unwind)
			set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
		if (state->should_exit || get_g_sig()->should_unwind)
			break ;
		if (!try_parse_tokens(state, parser, tt, prompt))
			break ;
	}
}

void	get_more_input_parser(t_shell *state,
							t_parser *parser,
							char **prompt,
							t_deque_tt *tt)
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
