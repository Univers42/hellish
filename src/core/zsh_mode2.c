/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zsh_mode2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 05:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 05:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"

/* An EXPLICIT dialect request -- `set -o zsh`, `emulate zsh` without -L.
**
** zsh options are global unless localoptions asks otherwise, so a request
** made by name must survive the end of whatever file it was typed in. But
** every frame_pop restores the dialect bit unconditionally (that is what
** keeps a sourced .zsh plugin from leaking the mode), so `set -o zsh` in
** ~/.hellishrc armed the dialect for exactly as long as the rc was being
** read and not one line longer -- the user's own prompt then rendered
** with the mode off, and PS1-as-the-zsh-parameter looked broken.
**
** Pinning rewrites the saved bit in every OPEN frame too, so no pending
** pop can quietly undo what was asked for by name. The automatic arming
** that comes from sourcing a .zsh file keeps using zsh_mode_swap and
** stays scoped, exactly as before.
*/
void	zsh_mode_pin(t_shell *state, bool on)
{
	t_call_frame	*f;
	size_t			i;

	zsh_mode_swap(state, on);
	if (!state)
		return ;
	i = 0;
	while (i < state->call_frames.len)
	{
		f = (t_call_frame *)vec_idx(&state->call_frames, i);
		f->zsh = on;
		if (on)
			f->setopt |= SETOPT_ZSH;
		else
			f->setopt &= ~SETOPT_ZSH;
		i++;
	}
}

/* `emulate` routes here: -L means frame-local (zsh's own meaning), and
   without it the request is global, i.e. pinned. */
int	zsh_mode_req(t_shell *state, bool on, bool local)
{
	if (local)
		zsh_mode_swap(state, on);
	else
		zsh_mode_pin(state, on);
	return (0);
}
