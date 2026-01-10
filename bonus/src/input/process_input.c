/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_input.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:17 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:17 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../shell.h"
#include <stdbool.h>

bool	is_empty_token_list(t_deque_tt *tokens)
{
	/* tokens->deqtok is the internal ft_deque */
	if (tokens->deqtok.len < 2)
		return (true);
	if (tokens->deqtok.len == 2
		&& ((t_token *)ft_deque_idx(&tokens->deqtok, 0))->tt == TT_NEWLINE)
		return (true);
	return (false);
}

bool	try_parse_tokens(t_state *state, t_parser *parser,
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
		set_cmd_status(state, (t_exe_res){.status = SYNTAX_ERR});
	free_ast(&state->tree);
	return (true);
}

static void	get_more_input_parser(t_state *state,
			t_parser *parser, char **prompt, t_deque_tt *tt)
{
	while (parser->res == RES_MoreInput || parser->res == RES_Init)
	{
		get_more_tokens(state, prompt, tt);
		if (g_should_unwind)
			set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
		if (state->should_exit || g_should_unwind)
			break ;
		if (!try_parse_tokens(state, parser, tt, prompt))
			break ;
	}
}

void	parse_and_execute_input(t_state *state)
{
	t_deque_tt	tt;
	char		*prompt;
	t_parser	parser;

	parser = (t_parser){.res = RES_Init};
	prompt = (char *)prompt_normal(state).ctx;
	/* initialize token deque wrapper */
	tt = (t_deque_tt){0};
	ft_deque_init(&tt.deqtok, 100, sizeof(t_token));
	tt.looking_for = 0;
	get_more_input_parser(state, &parser, &prompt, &tt);
	if (parser.res == RES_OK)
	{
		execute_top_level(state);
		free_ast(&state->tree);
	}
	if (g_should_unwind)
		set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
	manage_history(state);
	free (parser.parse_stack.buff);
	parser.parse_stack = (t_vec_int){};
	free(tt.deqtok.buff);
	state->should_exit |= (g_should_unwind
			&& state->input_method != INP_READLINE)
		|| state->readline_buff.has_finished;
}
