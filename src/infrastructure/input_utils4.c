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

/* Old try_parse_tokens before heredoc extraction was added (kept for ref):
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
			set_cmd_status(state,
				(t_execution_state){.status = SYNTAX_ERR});
	return (free_ast(&state->tree), true);
} */

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

	xfree(state->hd_src);
	state->hd_src = NULL;
	state->hd_pos = 0;
	xfree(state->hd_stripped);
	state->hd_stripped = NULL;
	in = (char *)state->alias_exp.ctx;
	if (!in || !ft_strnstr(in, "<<", state->alias_exp.len))
		return ;
	if (!split_heredocs(in, &stripped, &bodies))
		return ;
	state->hd_src = bodies;
	state->hd_stripped = stripped;
	tokenizer(stripped, tt);
}

/* Parse the current token list and decide what to do next. Returns true if
   execution should proceed (RES_OK or RES_ERR — both consume the token list),
   false if the list was empty (caller resets and loops). On RES_GETMOREINPUT
   we generate a continuation prompt (e.g. "if > ") so the user knows why we
   are asking for another line. The heredoc extraction runs first to peel off
   any heredoc bodies that were accumulated inline during multi-line input. */
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
