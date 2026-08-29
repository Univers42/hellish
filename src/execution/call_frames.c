/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   call_frames.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 12:55:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/27 12:55:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"
#include "env.h"

/* FUNCNAME and BASH_SOURCE, from one stack.
**
** Before this, the shell knew only HOW DEEP it was (func_depth, source_depth)
** and never WHERE: a sourced file could not name itself, because $0 inside it
** is /usr/bin/hellish. Issue #71 calls that the single highest-leverage gap
** for plugins -- without it every module hardcodes $HOME/.hellish and the
** tree cannot be relocated or vendored into a repo. The idiom that has to
** work is:
**
**     plugin_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
**
** Both are published as real env arrays, but LAZILY: push and pop only mark
** the stack dirty, and the arrays are rebuilt when something actually reads
** them.
**
** Publishing eagerly cost two allocations per push and two per pop -- about
** 0.5-1 us on every function call, whether or not anything ever looked at
** FUNCNAME. That was invisible while func_lookup was an O(n) scan and became
** a visible share of a call once it was not (2000 calls: 3.6 ms -> 5.6 ms).
** Almost no shell code reads these variables at all, so almost all of that
** work was pure waste.
**
** The read hook is env_expand_n(), which is the single choke point for BOTH
** forms -- `$FUNCNAME` and `${BASH_SOURCE[0]}` alike, because
** expand_array_token() resolves its base variable through it. It guards on a
** bool, then a length, then one character, so a lookup of any other name
** pays three comparisons and no allocation.
**
** DIVERGENCE FROM BASH, deliberate: bash keeps BASH_SOURCE the same length
** as FUNCNAME plus a "main" entry. Here there is exactly one entry per live
** frame, innermost first. The element that matters -- [0] -- agrees with
** bash in every case the idiom above cares about. BASH_LINENO is not
** provided: tok_lineno() cannot resolve lines inside sourced text today
** (see src/execution/exec_lineno.c:97), so it would be a confident lie. */

/* Collect one field of the stack, innermost first, into a fresh array value.
   An empty stack yields an EMPTY array rather than nothing: bash leaves
   FUNCNAME unset outside a function, and there is no public env-unset here
   (env_drop_entry is static to try_unset.c). ${FUNCNAME[0]} is empty and
   ${#FUNCNAME[@]} is 0 either way, which is what every real test looks at. */
static char	*frames_collect(t_shell *state, bool want_func)
{
	char			*elems[64];
	t_call_frame	*f;
	size_t			n;
	size_t			i;

	n = 0;
	i = state->call_frames.len;
	while (i-- > 0 && n < 64)
	{
		f = (t_call_frame *)vec_idx(&state->call_frames, i);
		if (want_func && f->func)
			elems[n++] = f->func;
		else if (!want_func && f->src)
			elems[n++] = f->src;
	}
	return (arr_from_elems(elems, (int)n, NULL));
}

/* Rebuild both arrays from the stack. Called from env_expand_n on the first
   read after a push or pop -- never from the call path itself. */
void	frames_sync(t_shell *state)
{
	state->frames_dirty = false;
	env_set(&state->env, env_create(ft_strdup("FUNCNAME"),
			frames_collect(state, true), false));
	env_set(&state->env, env_create(ft_strdup("BASH_SOURCE"),
			frames_collect(state, false), false));
}

/* Push a frame. `func` is NULL for a `source`, `src` is NULL when nothing is
   known about the file (a function defined interactively, then called). */
void	frame_push(t_shell *state, const char *func, const char *src)
{
	t_call_frame	f;

	f.func = NULL;
	f.src = NULL;
	if (func)
		f.func = ft_strdup(func);
	if (src)
		f.src = ft_strdup(src);
	if (!state->call_frames.elem_size)
		state->call_frames.elem_size = sizeof(t_call_frame);
	vec_push(&state->call_frames, &f);
	state->frames_dirty = true;
}

void	frame_pop(t_shell *state)
{
	t_call_frame	*f;

	if (state->call_frames.len)
	{
		f = (t_call_frame *)vec_idx(&state->call_frames,
				state->call_frames.len - 1);
		xfree(f->func);
		xfree(f->src);
		state->call_frames.len--;
	}
	state->frames_dirty = true;
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
