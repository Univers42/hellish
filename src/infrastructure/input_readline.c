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

/* Swap the raw input buffer for the history-expanded version. We also set
   input_expanded so manage_history() later saves the expanded form — that is
   the form the user effectively typed and the one that makes sense in history.
*/
static void	apply_expansion(t_shell *state, char *expanded)
{
	xfree(state->input.ctx);
	vec_init(&state->input);
	state->input.elem_size = 1;
	vec_push_str(&state->input, expanded);
	state->input_expanded = true;
}

/* Expand !! / !n / ^old^new style history references in what was just read.
   We only swap the buffer when expansion actually changed something (and
   produced a non-empty result) so a literal '!' in a word never triggers it. */
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
		xfree(expanded);
	xfree(raw);
}

/* Allocate the ring buffer and the input accumulator on first use. We defer
   the alloc to here rather than shell_init() so the memory is not wasted when
   running as a non-interactive script (INP_FILE path never calls readline). */
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

/* Read one logical line into state->input via the buffered readline layer,
   consuming and freeing *prompt. Returns 0 on success, 1 on EOF, 2 on ^C.
   If history expansion is active, the raw buffer is rewritten in-place before
   the caller sees it — so the tokenizer only ever sees the expanded form. */
int	readline_cmd(t_shell *state, char **prompt)
{
	int	stat;

	init_rl_bufs(state);
	stat = buff_readline(state, &state->input, *prompt);
	xfree(*prompt);
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
