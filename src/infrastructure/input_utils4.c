/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils4.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:31:45 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:31:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

bool try_parse_tokens(t_shell *state, t_parser *parser,
					  t_deque_tt *tt, char **prompt)
{
	if (is_empty_token_list(tt))
	{
		buff_readline_reset(&state->readline_buff);
		return (false);
	}
	parser->parse_stack.len = 0;
	state->tree = parse_tokens(state, parser, tt);
#if TRACE_DEBUG
	ft_eprintf("%s: debug: parser.res=%d\n", state->context, (int)parser->res);
#endif
	if (parser->res == RES_OK)
		return (true);
	else if (parser->res == RES_MoreInput)
	{
		*prompt = (char *)prompt_more_input(parser).ctx;
	}
	else if (parser->res == RES_FatalError)
	{
		/* Only set SYNTAX_ERR if status wasn't already set by the parser
		   (e.g., arithmetic errors set status to 1) */
		if (state->last_cmd_status_res.status == 0)
			set_cmd_status(state, (t_exe_res){.status = SYNTAX_ERR});
	}
	free_ast(&state->tree);
	return (true);
}

