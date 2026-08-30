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

/* The file the innermost frame belongs to, BORROWED -- the read-only twin of
   frame_src_dup, for $0 on the hot expansion path where an allocation per
   read would be pure waste. Same lifetime guarantee as frame_func_name: the
   frame owns the string for the whole call. */
const char	*frame_src_name(t_shell *state)
{
	t_call_frame	*f;
	size_t			i;

	i = state->call_frames.len;
	while (i-- > 0)
	{
		f = (t_call_frame *)vec_idx(&state->call_frames, i);
		if (f->src)
			return (f->src);
	}
	return (NULL);
}

/* Rebind $0 to the file being sourced, zsh-style, and hand back the old
** value for the caller to restore.
**
** zsh does not special-case $0 in a sourced file: it REBINDS the parameter,
** which is why `0="${...}"` inside a plugin works at all -- the standard
** preamble is three assignments that refine it:
**
**     0="${${ZERO:-${0:#$ZSH_ARGZERO}}:-${(%):-%N}}"
**     0="${${(M)0:#/[*]}:-$PWD/$0}"
**     typeset -g my_dir="${0:A:h}"
**
** Reading the frame at expansion time instead would discard all three and
** answer the same path every time -- which reads as the assignments having
** worked, because the value is still a plausible path.
**
** Returns the previous value (owned by the caller) or NULL when $0 was
** unset, which is what zsh_zero_restore expects back. */
char	*zsh_zero_bind(t_shell *state, const char *src)
{
	t_env	*e;
	char	*old;

	if (!zsh_mode(state) || !src)
		return (NULL);
	old = NULL;
	e = env_nget(&state->env, "0", 1);
	if (e && e->value)
		old = ft_strdup(e->value);
	env_set(&state->env, env_create(ft_strdup("0"),
			ft_strdup(src), false));
	return (old);
}

/* Put $0 back. A NULL `old` means it was unset before, which cannot happen
   in practice -- init.c seeds it -- so the value is simply left alone
   rather than inventing an unset that nothing else would produce. */
void	zsh_zero_restore(t_shell *state, char *old)
{
	if (!old)
		return ;
	env_set(&state->env, env_create(ft_strdup("0"), old, false));
}

/* A fresh copy of the file the innermost frame belongs to -- what a function
   being defined right now should record as its origin, so BASH_SOURCE[0]
   inside it later names where it was WRITTEN, not where it was called from.
   Returns a heap string the caller owns, or NULL at top level. Hands back a
   copy rather than the borrowed pointer because both call sites store it,
   and the frame it came from is gone by then. */
char	*frame_src_dup(t_shell *state)
{
	t_call_frame	*f;
	size_t			i;

	i = state->call_frames.len;
	while (i-- > 0)
	{
		f = (t_call_frame *)vec_idx(&state->call_frames, i);
		if (f->src)
			return (ft_strdup(f->src));
	}
	return (NULL);
}
