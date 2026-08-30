/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_array4.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 02:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 02:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "libft.h"

/* Append `n` element strings at consecutive indices starting at *w. */
static void	splice_put(t_string *out, char **elems, int n, long *w)
{
	int	i;

	i = 0;
	while (i < n)
	{
		rec_append(out, (*w)++, elems[i], (int)ft_strlen(elems[i]));
		i++;
	}
}

/* Elements `r.lo`..`r.hi` of `old` replaced by the `n` strings in `elems`,
** everything after them RENUMBERED.  zsh's `a[i]=(x y)` and `a[lo,hi]=(...)`
** are the same operation over a one-element and a wider range -- and with
** n == 0, `a[i]=()`, which is how a plugin pops a stack: the element goes
** and the array closes up behind it.
**
** The renumbering is the whole job.  arr_without leaves a hole (indices 0
** and 2 after removing 1), which still reads correctly through ${a[@]} but
** makes ${a[$#a]} -- "the last element", found by COUNT -- name an index
** that is not there any more.  A pop would then return the empty string,
** and dirhistory reads that as "someone overwrote our variable".
**
** Three edges, all of them zsh's own rules rather than choices:
**   hi < lo   INSERTS at lo and removes nothing.  `a=(1 2 3); a[3,2]=(P)`
**             splices P in ahead of element 3 -- an empty range is a
**             position, not an error.
**   lo > end  PADS with empty elements: `a=(1 2 3); a[9]=(x)` gives nine
**             elements, five of them empty.
**   hi > end  takes everything from lo onwards, no complaint.
*/
/* Carry one existing element over at the next write index.  A gap in the
   source becomes an EMPTY element rather than vanishing, which is what makes
   assigning past the end pad instead of leaving a hole. */
static void	splice_keep(t_string *out, const char *old, long i, long *w)
{
	char	*e;

	e = arr_get_idx(old, i);
	if (!e)
		e = ft_strdup("");
	rec_append(out, (*w)++, e, (int)ft_strlen(e));
	xfree(e);
}

char	*arr_splice(const char *old, t_slice r, char **elems, int n)
{
	t_string	out;
	long		last;
	long		i;
	long		w;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_MAGIC);
	last = arr_max_idx(old);
	if (r.lo > last)
		last = r.lo;
	i = -1;
	w = 0;
	while (++i <= last)
	{
		if (i == r.lo)
			splice_put(&out, elems, n, &w);
		if (i >= r.lo && i <= r.hi)
			continue ;
		if (arr_is(old))
			splice_keep(&out, old, i, &w);
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}
