/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_rprompt.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 02:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 15:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"
#include "sh_input.h"

/* RPROMPT -- zsh's right-side prompt, issue #91.
**
** The first version appended the rendered right prompt to readline's own
** prompt string, inside one \001...\002 guard: save cursor, jump to the
** right margin, print, restore. It assumed readline would count none of
** it. readline does not nest guards. A coloured RPROMPT already carries
** its own \001/\002 pairs from %F{..}%f, so readline stopped ignoring at
** the FIRST \002, counted the rest as visible columns, and copied the
** inner markers to the terminal. Every history recall then redrew from a
** prompt readline believed nine columns wider than it was -- the cursor
** drift of the field reports, reproduced cell by cell in
** tests/prompt_drift_matrix_test.py. The plain, uncoloured case had a
** smaller defect of its own: readline's clear-to-end-of-line on a redraw
** erased the clock and nothing ever repainted it.
**
** So the right prompt is no longer part of readline's prompt at all. This
** file only RENDERS it -- once per primary prompt, into t_rl -- with the
** width markers stripped, since the text will be written straight to the
** tty. The painting happens inside the editor, from the redisplay hook
** (src/platform/posix/rl_rprompt.c), after every redraw readline makes:
** it survives ↑/↓ and never enters readline's width model.
**
** The format string gets EXACT zsh semantics (strict reader): RPROMPT is
** a zsh variable with no bash ancestry, so there is no legacy spelling to
** protect. Skipped on a dumb terminal, where cursor movement is not a
** vocabulary, and outside an interactive read. */

/* Cursor games need a terminal that plays them. */
static bool	term_is_dumb(t_shell *state)
{
	char	*term;

	term = env_expand(state, "TERM");
	return (term && !ft_strcmp(term, "dumb"));
}

/* The rendered right prompt, through the same two-stage pipeline the left
   one uses: the zsh reader, then the backslash renderer. */
static t_string	rp_render(t_shell *state, char *rp)
{
	t_string	conv;
	t_string	txt;

	conv = zsh_to_ps1(state, rp, true);
	txt = ps1_render(state, (char *)conv.ctx);
	xfree(conv.ctx);
	return (txt);
}

/* Drop the \001/\002 width markers in place. They exist to guide readline,
   and this text never reaches readline -- on a terminal they print as
   noise (and used to, see the file comment). */
static void	rp_strip(t_string *txt)
{
	char	*s;
	size_t	r;
	size_t	w;

	s = (char *)txt->ctx;
	r = 0;
	w = 0;
	while (r < txt->len)
	{
		if (s[r] != '\001' && s[r] != '\002')
			s[w++] = s[r];
		r++;
	}
	s[w] = '\0';
	txt->len = w;
}

/* Forget the rendered right prompt: nothing gets painted until the next
   primary prompt renders one. prompt_more_input calls this too -- zsh
   shows no right prompt on a continuation row, and neither do we. */
void	rprompt_clear(t_shell *state)
{
	if (state->rl.rp_txt.ctx)
		xfree(state->rl.rp_txt.ctx);
	state->rl.rp_txt = (t_string){0};
	state->rl.rp_w = 0;
	state->rl.rp_painted = false;
}

/* Render RPROMPT (RPS1 is zsh's other spelling) for this prompt and park it
   in t_rl for the editor to paint. Re-read every prompt, so `unset RPROMPT`
   or a theme switch takes effect on the very next one. */
void	rprompt_render(t_shell *state)
{
	char		*rp;
	t_string	txt;

	rprompt_clear(state);
	rp = env_expand(state, "RPROMPT");
	if (!rp || !*rp)
		rp = env_expand(state, "RPS1");
	if (!rp || !*rp || term_is_dumb(state) || state->metinp != INP_RL)
		return ;
	txt = rp_render(state, rp);
	if (!txt.ctx)
		return ;
	rp_strip(&txt);
	state->rl.rp_w = visible_width_cstr((char *)txt.ctx);
	state->rl.rp_txt = txt;
	if (state->rl.rp_w <= 0)
		rprompt_clear(state);
}
