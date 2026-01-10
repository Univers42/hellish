/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_more_tokens.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:32 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:32 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../shell.h"
#include <stdbool.h>

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


void	get_more_tokens(t_state *state, char **prompt, t_deque_tt *tt)
{
	int		stat;

	while (*prompt)
	{
		stat = readline_cmd(state, prompt);
		if (stat == 1)
		{
			if (tt->looking_for && state->input.len)
				ft_eprintf("%s: unexpected EOF while looking for "
					"matching `%c'\n", state->context, tt->looking_for);
			else if (state->input.len)
				ft_eprintf("%s: syntax error: unexpected end of file\n",
					state->context);
			if (state->input_method == INP_READLINE)
				ft_eprintf("exit\n");
			if (!state->last_cmd_status_res.status && state->input.len)
				set_cmd_status(state, (t_exe_res){.status = SYNTAX_ERR});
			state->should_exit = true;
		}
		if (stat)
			return ;
		*prompt = (extend_bs(state), tokenizer((char *)state->input.ctx, tt));
		if (*prompt)
			*prompt = ft_strdup(*prompt);
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
