/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_rprompt.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 02:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 02:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"
#include "sh_input.h"

/* RPROMPT -- zsh's right-side prompt, issue #91.
**
** Emulated the way bash themes have always faked it: a zero-width guarded
** block appended AFTER the prompt that saves the cursor, jumps to the
** right margin, prints, and jumps back. readline counts none of it (the
** \001/\002 guards make it invisible to the width model), and the cursor
** lands back at the input point as if nothing happened. Typing far enough
** simply overwrites it -- zsh auto-hides at that moment; overwriting is
** the readline-world equivalent, and both leave the input intact.
**
** The format string gets EXACT zsh semantics (strict reader): RPROMPT is
** a zsh variable with no bash ancestry, so there is no legacy spelling to
** protect. Skipped when it does not fit, and on a dumb terminal, where
** cursor movement is not a vocabulary. */

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

t_string	rprompt_wrap(t_shell *state, t_string base)
{
	char		seq[24];
	char		*rp;
	t_string	txt;
	int			w;
	int			cols;

	rp = env_expand(state, "RPROMPT");
	if (!rp || !*rp || term_is_dumb(state) || state->metinp != INP_RL)
		return (base);
	txt = rp_render(state, rp);
	w = visible_width_cstr((char *)txt.ctx);
	cols = get_cols();
	if (txt.ctx && w > 0 && cols > w + 1)
	{
		snprintf(seq, sizeof(seq), "\001\033[s\033[%dG", cols - w + 1);
		vec_push_str(&base, seq);
		vec_push_str(&base, (char *)txt.ctx);
		vec_push_str(&base, "\033[u\002");
		vec_push_char(&base, '\0');
		base.len--;
	}
	return (xfree(txt.ctx), base);
}
