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
** Both variables are published as real env arrays on push and pop rather
** than synthesised at read time, because the read path (env_get) is far
** hotter than the call path and must not grow a special case.
**
** ponytail: publishing eagerly costs two allocations per push and two per
** pop -- measured at roughly 0.5-1 us per function call, which was invisible
** while func_lookup was an O(n) scan and is now a visible share of a call
** (2000 calls: 3.6 ms before this, 5.6 ms after, on an otherwise flat
** curve). Upgrade path if it ever matters: keep the stack, set a dirty flag
** here, and publish lazily on first read. That needs a read hook, and
** env_get() takes a t_vec_env* with no t_shell* to hang the flag off, so it
** is a signature change across a very hot path -- not worth it until a
** profile says so.
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

static void	frames_publish(t_shell *state)
{
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
	frames_publish(state);
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
	frames_publish(state);
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
