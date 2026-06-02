/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_readline.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:31:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:53:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"
#include "helpers.h"

char	*expand_history(t_shell *state, const char *input);

static void	apply_expansion(t_shell *state, char *expanded)
{
	free(state->input.ctx);
	vec_init(&state->input);
	state->input.elem_size = 1;
	vec_push_str(&state->input, expanded);
	state->input_expanded = true;
}

static void	expand_hist_in_input(t_shell *state)
{
	char	*expanded;
	char	*raw;

	raw = ft_strndup((char *)state->input.ctx, state->input.len);
	if (!raw)
		return ;
	expanded = expand_history(state, raw);
	if (expanded && ft_strcmp(expanded, raw) != 0 && expanded[0] != '\0')
		apply_expansion(state, expanded);
	if (expanded)
		free(expanded);
	free(raw);
}

static void	init_rl_bufs(t_shell *state)
{
	if (!state->rl.buff.ctx)
	{
		vec_init(&state->rl.buff);
		state->rl.buff.elem_size = 1;
	}
	if (!state->input.ctx)
	{
		vec_init(&state->input);
		state->input.elem_size = 1;
	}
}

int	readline_cmd(t_shell *state, char **prompt)
{
	int	stat;

	init_rl_bufs(state);
	stat = buff_readline(state, &state->input, *prompt);
	free(*prompt);
	*prompt = 0;
	if (stat == 0)
		return (1);
	if (stat == 2)
	{
		if (state->metinp != INP_RL)
			state->should_exit = true;
		return (2);
	}
	if (state->hist.hist_active && state->input.ctx && state->input.len > 0)
		expand_hist_in_input(state);
	return (0);
}
