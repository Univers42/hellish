/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:12 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 18:22:25 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

static void	prepare_parser_and_prompt(t_shell *state,
									t_parser *parser,
									t_deque_tt *tt,
									char **prompt)
{
	t_string	p;

	(void)state;
	*parser = (t_parser){.res = RES_Init};
	vec_init(&parser->parse_stack);
	parser->parse_stack.elem_size = sizeof(int);
	p = prompt_normal();
	*prompt = ft_strdup(p.ctx);
	free(p.ctx);
	*tt = (t_deque_tt){0};
	deque_init(&tt->deqtok, 100, sizeof(t_token));
	tt->looking_for = 0;
}

static void	finalize_parser_and_cleanup(t_shell *state,
										t_parser *parser,
										t_deque_tt *tt,
										char *prompt)
{
	if (parser->res == RES_OK)
	{
		execute_top_level(state);
		free_ast(&state->tree);
	}
	if (get_g_sig()->should_unwind)
		set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
	manage_history(state);
	if (parser->parse_stack.ctx)
		free(parser->parse_stack.ctx);
	parser->parse_stack = (t_vec_int){};
	if (prompt)
		free(prompt);
	if (tt->deqtok.buff)
		free(tt->deqtok.buff);
	state->should_exit |= (get_g_sig()->should_unwind
			&& state->input_method != INP_READLINE)
		|| state->readline_buff.has_finished;
}

void	parse_and_execute_input(t_shell *state)
{
	t_deque_tt		tt;
	char			*prompt;
	t_parser		parser;

	prepare_parser_and_prompt(state, &parser, &tt, &prompt);
	get_more_input_parser(state, &parser, &prompt, &tt);
	finalize_parser_and_cleanup(state, &parser, &tt, prompt);
}
