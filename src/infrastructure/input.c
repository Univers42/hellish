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

#include "input_private.h"
#include "helpers.h"

static void	handle_eof_or_error(t_shell *state, t_deque_tok *tt)
{
	if (tt->looking_for && state->input.len)
		ft_eprintf("%s: unexpected EOF while looking for "
			"matching `%c'\n",
			state->ctx, tt->looking_for);
	else if (state->input.len)
		ft_eprintf("%s: syntax error: unexpected end of file\n",
			state->ctx);
	if (state->metinp == INP_RL)
		ft_eprintf("exit\n");
	if (!state->last_cmd_st_exe.status && state->input.len)
		set_cmd_status(state, (t_execution_state){.status = SYNTAX_ERR});
	state->should_exit = true;
}

static void	update_prompt(t_shell *state, char **prompt, t_deque_tok *tt)
{
	*prompt = (extend_bs(state), tokenizer((char *)state->input.ctx, tt));
	if (*prompt)
		*prompt = ft_strdup(*prompt);
}

int	get_more_tokens(t_shell *state, char **prompt, t_deque_tok *tt)
{
	int	stat;

	while (*prompt)
	{
		stat = readline_cmd(state, prompt);
		if (stat == 1)
			handle_eof_or_error(state, tt);
		if (stat)
			return (stat);
		update_prompt(state, prompt, tt);
	}
	return (0);
}
