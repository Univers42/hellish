/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array_splice.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 02:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 02:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "sh_input.h"

void	fatal_input_error(t_shell *state, int code);

/* zsh's `a[i]=(...)` -- assigning a LIST to one element.
**
**     dirhistory_past[$#dirhistory_past]=()
**
** is oh-my-zsh's dirhistory popping its stack, and it is the shape that
** decides whether that plugin parses at all.  The list replaces the element
** and the array closes up behind it, so an empty list removes and a
** two-element list makes the array grow by one.
**
** bash has no such form -- `a[2]=(x)` is a syntax error there -- so the
** parser only recognises the target in the zsh dialect (parse_array.c) and
** this never runs for bash input.
*/

/* Value of the subscript text between the brackets, as the 0-based index
   the store uses.  `text` starts just after '[' and ends at the ']'.
     Word-expanded and then evaluated arithmetically -- the same two steps
   in the same order as the scalar `a[i]=v` path, so that `a[$((n+1))]` and
   `a[$#a]` mean the same thing whichever kind of value is being assigned.
     `count` comes from the caller because a negative subscript counts back
   from the end, and only the caller has the array. */
long	elem_sub_index(t_shell *state, char *text, long count)
{
	char	*rb;
	char	*word;
	char	*res;
	long	idx;

	rb = ft_strrchr(text, ']');
	if (!rb)
		return (-1);
	word = expand_param_word(state, text, (int)(rb - text), false);
	res = arith_expand(state, word, (int)ft_strlen(word));
	idx = 0;
	if (res)
		idx = ft_atoi(res);
	xfree(word);
	xfree(res);
	return (sub_to_index(state, idx, count));
}

/* A subscript that names no possible element: `a[-1]` counted back past the
   start, or zsh's `a[0]` where counting begins at 1.  Writing to a different
   element than the script named is the one outcome worse than stopping, so
   this never assigns -- and fatal_input_error decides how far the stop
   reaches (the shell for -c and scripts, only the file when sourced). */
void	bad_subscript(t_shell *state, const char *name, const char *sub)
{
	if (zsh_arrays(state))
		ft_eprintf("%s: %s: assignment to invalid subscript range\n",
			state->ctx, name);
	else
		ft_eprintf("%s: %s[%s]: bad array subscript\n",
			state->ctx, name, sub);
	set_cmd_status(state, create_exec_state(1, false));
	fatal_input_error(state, 1);
}

/* The assignment target as a RANGE.  `a[i]=(...)` and zsh's
   `a[lo,hi]=(...)` are one operation over a narrow and a wide run, so both
   arrive here as a t_slice and arr_splice never has to ask which was
   written.  A plain index becomes the one-element range [i,i]. */
static t_slice	target_slice(t_shell *state, char *text, const char *old)
{
	t_slice	r;
	char	*rb;

	r.lo = SLICE_NONE;
	r.hi = 0;
	rb = ft_strrchr(text, ']');
	if (rb)
		r = zsh_slice_bounds(state, text, (int)(rb - text), old);
	if (r.lo != SLICE_NONE)
		return (r);
	r.lo = elem_sub_index(state, text, arr_count(old));
	r.hi = r.lo;
	return (r);
}

/* The array as it stands, for the paths that must leave it ALONE: an
   out-of-range subscript names no destination, so the old value is written
   back unchanged rather than the assignment landing somewhere else. */
static char	*keep_unchanged(t_shell *state, t_env *ev, const char *old)
{
	bad_subscript(state, ev->key, "");
	if (!old)
		return (ft_strdup(""));
	return (ft_strdup(old));
}

/* Splice the expanded list into `ev->key`'s array at the subscript the key
   carries, and truncate the key to the bare name.  False for a plain
   `a=(...)` target, which the ordinary builder handles.
     An associative target is refused too.  zsh words that one differently
   ("attempt to set slice of associative array"); the status, the abort and
   the untouched value are the same, and inventing a third message shape for
   a case no plugin in the corpus reaches is not worth the branch. */
bool	splice_elem_assign(t_shell *state, t_env *ev, t_vec *args)
{
	char	*br;
	char	*old;
	t_slice	r;

	br = ft_strchr(ev->key, '[');
	if (!br)
		return (false);
	*br = '\0';
	old = env_expand(state, ev->key);
	r = target_slice(state, br + 1, old);
	if (assoc_is(old) || r.lo < 0)
		return (ev->value = keep_unchanged(state, ev, old), true);
	ev->value = arr_splice(old, r, (char **)args->ctx, (int)args->len);
	return (true);
}
