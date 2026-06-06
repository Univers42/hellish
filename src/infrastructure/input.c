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

/* Report EOF or truncated input with the right error to match bash's wording.
   If the tokenizer was waiting for a closing `, do:  or fi when EOF arrived,
   it says "looking for `X'"; otherwise "unexpected end of file". The "exit"
   echoed for interactive mode mirrors what bash prints when you hit ^D. */
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

/* After each successful readline, tokenize what we have so far and check
   whether the tokenizer needs more input (prompt != NULL means "keep going").
   extend_bs strips trailing backslash-newlines first so line continuation
   is resolved before the tokenizer ever sees the buffer. */
static void	update_prompt(t_shell *state, char **prompt, t_deque_tok *tt)
{
	*prompt = (extend_bs(state), tokenizer((char *)state->input.ctx, tt));
	if (*prompt)
		*prompt = ft_strdup(*prompt);
}

/* Keep reading lines until the tokenizer stops asking for more (prompt becomes
   NULL). Handles EOF and interrupt in the loop: EOF always breaks and returns
   the error code; interrupt passes straight up to the caller who knows how to
   clear the in-progress command and repaint the prompt. */
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
