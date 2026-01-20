/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_get_more_utils3.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 17:47:07 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 17:59:48 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

void	print_and_cleanup_tokens(t_shell *state,
									t_deque_tt *tt,
									char **prompt)
{
	debug_lexer_print_tokens(state, tt);
	debug_lexer_cleanup(state, tt, prompt);
}

// Fetch input and handle interrupts/EOF, 
// returns 1 for break, 2 for continue, 0 for proceed
static int	fetch_and_handle_input(t_shell *state,
									t_parser *parser,
									char **prompt,
									t_deque_tt *tt)
{
	int	s;

	(void)parser;
	s = get_more_tokens(state, prompt, tt);
	if (handle_eof(s, state))
		return (1);
	if (handle_interrupt(s, state, tt, prompt) == 2)
	{
		s = get_more_tokens(state, prompt, tt);
		if (handle_eof(s, state))
			return (1);
		if (handle_interrupt(s, state, tt, prompt) == 2)
			return (2);
	}
	return (0);
}

// Parse tokens, print AST, cleanup, and reset parser state
static void	parse_print_and_cleanup(t_shell *state,
									t_parser *parser,
									t_deque_tt *tt,
									char **prompt)
{
	t_ast_node	parsed;

	parser->parse_stack.len = 0;
	parsed = parse_tokens(state, parser, tt);
	debug_parser_print_ast(state, parser, parsed);
	debug_parser_cleanup(state, tt, prompt);
	parser->res = RES_Init;
}

void	debug_parser_loop(t_shell *state, t_parser *parser,
							char **prompt, t_deque_tt *tt)
{
	int	action;

	while (parser->res == RES_MoreInput || parser->res == RES_Init)
	{
		set_cmd_status(state, res_status(0));
		action = fetch_and_handle_input(state, parser, prompt, tt);
		if (action == 1)
			break ;
		if (action == 2)
			continue ;
		set_cmd_status(state, res_status(0));
		if (get_g_sig()->should_unwind)
			set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
		if (state->should_exit || get_g_sig()->should_unwind)
			break ;
		if (is_empty_token_list(tt))
		{
			buff_readline_reset(&state->readline_buff);
			continue ;
		}
		parse_print_and_cleanup(state, parser, tt, prompt);
	}
}
