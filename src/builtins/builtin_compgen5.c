/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compgen5.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Collected output for the compgen actions whose SOURCE ORDER is an
** implementation detail.
**
** bash sorts these and hellish did not, which is a divergence that hides
** on a small machine and appears on a big one: `compgen -v HOM` on a
** laptop with one matching variable is right by luck, and on a CI runner
** carrying three HOMEBREW_* variables it is wrong.  That is exactly how
** it was found -- the golden suite passed here and failed on arm64.
**
** Not every action sorts, and guessing would have been wrong in both
** directions.  Measured against bash 5.3.9:
**
**     -W wordlist   list order, NOT sorted   (`-W 'zeta alpha'` stays)
**     -v -b -k -a   sorted
**     -A function   sorted
**     -f -d         readdir order, NOT sorted
**
** So the collector is used only where bash sorts, and cg_emit still
** prints straight through everywhere else.
*/

/* Add `s` to the collection when it carries the prefix. */
void	cg_add(t_vec *out, const char *s, const char *pfx)
{
	char	*d;

	if (ft_strncmp((char *)s, (char *)pfx, ft_strlen((char *)pfx)) != 0)
		return ;
	if (out->elem_size == 0)
	{
		vec_init(out);
		out->elem_size = sizeof(char *);
	}
	d = ft_strdup((char *)s);
	vec_push(out, &d);
}

/* Sort, print, release; returns how many were printed so the caller can
   keep accumulating "did anything match at all". */
int	cg_flush(t_cgopt *o, t_vec *out)
{
	size_t	i;
	int		n;

	if (out->len > 1)
		ft_quicksort(out);
	i = 0;
	n = 0;
	while (i < out->len)
	{
		n += cg_print(o, ((char **)out->ctx)[i]);
		xfree(((char **)out->ctx)[i]);
		i++;
	}
	xfree(out->ctx);
	return (n);
}
