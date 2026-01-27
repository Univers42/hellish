/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_get_more_utils.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 17:33:06 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:23:04 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

void	reset_status_and_prompt(t_shell *state, char **prompt)
{
	t_string	p;

	set_cmd_status(state, res_status(0));
	if (*prompt)
		free(*prompt);
	{
		p = prompt_normal();
		*prompt = ft_strdup(p.ctx);
		free(p.ctx);
	}
}

void	handle_ctrl_c(t_shell *state, t_deque_tok *tt, char **prompt)
{
	buff_readline_reset(&state->rl);
	if (tt->deqtok.buff)
		deque_clear(&tt->deqtok, NULL);
	tt->looking_for = 0;
	if (state->input.ctx)
		state->input.len = 0;
	set_cmd_status(state, (t_execution_state){.status = 130, .ctrl_c = true});
	reset_status_and_prompt(state, prompt);
	buff_readline_update(&state->rl);
}

int	handle_eof(int s, t_shell *state)
{
	if (s == 1)
	{
		state->should_exit = true;
		return (1);
	}
	return (0);
}

int	handle_interrupt(int s, t_shell *state, t_deque_tok *tt, char **prompt)
{
	if (s == 2)
	{
		handle_ctrl_c(state, tt, prompt);
		return (2);
	}
	return (0);
}

void	debug_parser_print_ast(t_shell *state,
								t_parser *parser,
								t_ast_node parsed)
{
	if (parser->res == RES_OK || parser->res == RES_GETMOREINPUT)
		print_ast_dot(state, parsed);
	if (parser->res == RES_OK || parser->res == RES_ERR
		|| parser->res == RES_GETMOREINPUT)
		free_ast(&parsed);
	if (parser->res == RES_ERR)
		set_cmd_status(state, (t_execution_state){.status = SYNTAX_ERR});
}
