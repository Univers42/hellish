/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_ps2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 19:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/09/25 19:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "sh_input.h"
#include "env.h"

/* The prompt for a continuation line, every kind of it: an open quote, a
   backslash-newline, a heredoc body, an unfinished compound command. In
   bash PS2 is the one continuation prompt, so a user-set PS2 wins
   everywhere and renders with the full escape set; unset, `fallback` is
   the built-in label naming what is still open ("dquote> ", "if> ").
   It used to be read only for an unfinished compound: the lexer and the
   heredoc reader handed readline a fixed label, so PS2 looked ignored.

   Only an interactive read shows a prompt, so only it renders PS2: a
   script pays nothing for it, and a PS2 with a side effect ($((n+=1)))
   runs no more often than in bash, which never expands it there.

   A continuation row is not the primary prompt. The right prompt is
   dropped (zsh paints none there) and ps1_read goes false, which keeps
   the idle repaint -- it rewrites the PS1 rows above the cursor -- off
   this row. The lexer paths skipped both, so RPROMPT was painted after
   "dquote> ". Returns a new string for readline_cmd to free. */
char	*prompt_ps2(t_shell *state, const char *fallback)
{
	const char	*ps2;

	rprompt_clear(state);
	state->rl.ps1_read = false;
	if (state->metinp == INP_RL)
	{
		ps2 = env_expand(state, "PS2");
		if (ps2 && *ps2)
			return ((char *)ps1_render(state, ps2).ctx);
	}
	return (ft_strdup(fallback));
}
