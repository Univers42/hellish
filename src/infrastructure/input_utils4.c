/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils4.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:31:45 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 14:07:05 by marvin           ###   ########.fr       */
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

/* Heredocs inside a compound command (function/loop/if/braces) have their body
   accumulated into state->input by the construct's continuation, so the body
   would otherwise be parsed as commands. Pull those bodies out up front (only
   the ones already terminated by their delimiter; a simple top-level heredoc is
   still read live from the stream) and feed them via state->hd_src. */
static void	extract_input_heredocs(t_shell *state, t_deque_tok *tt)
{
	char	*stripped;
	char	*bodies;
	char	*in;

	free(state->hd_src);
	state->hd_src = NULL;
	state->hd_pos = 0;
	free(state->hd_stripped);
	state->hd_stripped = NULL;
	in = (char *)state->input.ctx;
	if (!in || !ft_strnstr(in, "<<", state->input.len))
		return ;
	if (!split_heredocs(in, &stripped, &bodies))
		return ;
	state->hd_src = bodies;
	state->hd_stripped = stripped;
	tokenizer(stripped, tt);
}

bool	try_parse_tokens(t_shell *state, t_parser *parser,
							t_deque_tok *tt, char **prompt)
{
	if (is_empty_token_list(tt))
		return (buff_readline_reset(&state->rl), false);
	parser->parse_stack.len = 0;
	extract_input_heredocs(state, tt);
	reclassify_keywords(tt);
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
