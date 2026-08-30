/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_slice2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* What a subscript counts against: ELEMENTS for an array, CHARACTERS for a
   scalar.  zsh subscripts a plain string by character -- `x=hello;
   ${x[2,3]}` is "el" -- so the same negative-wrapping arithmetic has to
   resolve against a different total depending on what the name holds. */
long	zsh_slice_universe(const char *val)
{
	if (arr_is(val))
		return ((long)arr_count(val));
	if (!val)
		return (0);
	return ((long)ft_strlen(val));
}

/* How many items the slice actually covers once the end of the container is
   taken into account.  This is what ${#a[2,3]} answers, and it is not
   hi - lo + 1: `a[2,99]` on five elements covers four, not ninety-eight. */
long	zsh_slice_len(t_slice r, long count)
{
	long	hi;

	if (r.lo < 0 || r.lo >= count)
		return (0);
	hi = r.hi;
	if (hi >= count)
		hi = count - 1;
	if (hi < r.lo)
		return (0);
	return (hi - r.lo + 1);
}

/* `a[lo,hi]=X` -- a SCALAR right-hand side over a range.  zsh replaces the
   whole run with that ONE element: `a=(1 2 3 4 5); a[2,3]=X` leaves four.
   The list form `a[lo,hi]=(...)` is the same splice with more elements, so
   both end at arr_splice and neither carries a rule of its own. */
void	zsh_slice_set(t_env *ret, const char *old, t_slice r)
{
	char	*res;

	res = arr_splice(old, r, &ret->value, 1);
	xfree(ret->value);
	ret->value = res;
}

/* The slice's value as one string, joined the way "${a[*]}" joins: with
   IFS[0], or with nothing at all when IFS is empty.  An unquoted slice is
   then split again by the ordinary word-splitting pass, which is how
   `for e in ${a[2,3]}` sees two words -- the same route "${a[*]}" already
   takes, no separate machinery. */
char	*zsh_slice_str(t_shell *state, const char *val, t_slice r)
{
	char	*ifs;
	long	n;

	if (!val || r.lo < 0 || r.hi < r.lo)
		return (ft_strdup(""));
	if (arr_is(val))
	{
		ifs = env_get_ifs(&state->env);
		return (arr_join_range(val, ifs[0], r));
	}
	n = zsh_slice_len(r, (long)ft_strlen(val));
	if (n <= 0)
		return (ft_strdup(""));
	return (ft_strndup((char *)val + r.lo, (size_t)n));
}
