/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils4.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:31:45 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:20:47 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

//bool	try_parse_tokens(t_shell *state, t_parser *parser,
//							t_deque_tok *tt, char **prompt)
//{
//	if (is_empty_token_list(tt))
//		return (buff_readline_reset(&state->rl), false);
//	parser->parse_stack.len = 0;
//	state->tree = parse_tokens(state, parser, tt);
//#if TRACE_DEBUG
//	ft_eprintf("%s: debug: parser.res=%d\n",
//		state->ctx, (int)parser->res);
//#endif
//	if (parser->res == RES_OK)
//		return (true);
//	else if (parser->res == RES_GETMOREINPUT)
//		*prompt = (char *)prompt_more_input(parser).ctx;
//	else if (parser->res == RES_ERR)
//		if (state->last_cmd_st_exe.status == 0)
//			set_cmd_status(state, (t_execution_state){.status = SYNTAX_ERR});
//	return (free_ast(&state->tree), true);
//}

bool	try_parse_tokens(t_shell *state, t_parser *parser,
							t_deque_tok *tt, char **prompt)
{
	if (is_empty_token_list(tt))
		return (buff_readline_reset(&state->rl), false);
	parser->parse_stack.len = 0;
	state->tree = parse_tokens(state, parser, tt);
	if (parser->res == RES_OK)
		return (true);
	else if (parser->res == RES_GETMOREINPUT)
		*prompt = (char *)prompt_more_input(parser).ctx;
	else if (parser->res == RES_ERR)
		if (state->last_cmd_st_exe.status == 0)
			set_cmd_status(state, (t_execution_state){.status = SYNTAX_ERR});
	return (free_ast(&state->tree), true);
}
