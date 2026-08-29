/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   call_frames2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 12:55:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/27 12:55:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

/* The name of the function whose call we are inside, borrowed, or NULL at
   top level or inside a plain `source`.
**
** This is zsh's $0. In zsh a function's $0 is its own NAME, not the shell's;
** in bash it stays the shell. The difference is the whole mechanism behind
** oh-my-zsh's colored-man-pages:
**
**     function man dman debman { colored $0 "$@" }
**
** -- one body, three names, and $0 is how it tells which one was called.
** Under bash rules all three would colour `man`.
**
** Borrowed, not copied: frames own their strings for the whole call, which
** is the same lifetime guarantee an env value already has, and $0 is read
** on a hot path. The frame's own comment explains why that ownership is not
** negotiable -- a function can free its own definition mid-call. */
const char	*frame_func_name(t_shell *state)
{
	t_call_frame	*f;
	size_t			i;

	i = state->call_frames.len;
	while (i-- > 0)
	{
		f = (t_call_frame *)vec_idx(&state->call_frames, i);
		if (f->func)
			return (f->func);
	}
	return (NULL);
}
