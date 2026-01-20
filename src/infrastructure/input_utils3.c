/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:31:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:31:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

bool ends_with_bs_nl(t_string s)
{
	size_t i;
	bool unterminated;

	if (s.len == 0)
		return (false);
	i = s.len;
	unterminated = false;
	if (((char *)s.ctx)[--i] != '\n')
		return (false);
	while (i > 0)
	{
		i--;
		if (((char *)s.ctx)[i] == '\\')
			unterminated = !unterminated;
		else
			break;
	}
	return (unterminated);
}

void extend_bs(t_shell *state)
{
	char *prompt;

	while (ends_with_bs_nl(state->input))
	{
		vec_pop(&state->input);
		vec_pop(&state->input);
		prompt = ft_strdup("> ");
		if (readline_cmd(state, &prompt))
			return;
	}
}

bool is_empty_token_list(t_deque_tt *tokens)
{
	/* tokens->deqtok is the internal t_deque */
	if (tokens->deqtok.len < 2)
		return (true);
	if (tokens->deqtok.len == 2 && ((t_token *)deque_idx(&tokens->deqtok, 0))->tt == TT_NEWLINE)
		return (true);
	return (false);
}

int readline_cmd(t_shell *state, char **prompt)
{
	int stat;

	if (!state->readline_buff.buff.ctx)
	{
		vec_init(&state->readline_buff.buff);
		state->readline_buff.buff.elem_size = 1;
	}
	if (!state->input.ctx)
	{
		vec_init(&state->input);
		state->input.elem_size = 1;
	}
	stat = buff_readline(state, &state->input, *prompt);
	free(*prompt);
	*prompt = 0;
	if (stat == 0)
	{
		return (1);
	}
	if (stat == 2)
	{
		if (state->input_method != INP_READLINE)
			state->should_exit = true;
		return (2);
	}
	return (0);
}
