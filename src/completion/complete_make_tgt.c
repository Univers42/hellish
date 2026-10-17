/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_tgt.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Which target names are worth offering, and the list they go in.  See
   complete_make.c. */

#include "complete_make.h"

/* [A-Za-z0-9_./+-]+, not starting with '.', hence no %, no $, no .PHONY. */
static int	mk_ok(const char *w, size_t n)
{
	size_t	i;

	if (n == 0 || w[0] == '.')
		return (0);
	i = 0;
	while (i < n)
	{
		if (!ft_isalnum(w[i]) && !ft_strchr("_./+-", w[i]))
			return (0);
		i++;
	}
	return (1);
}

/* Push a copy of w unless the name is already there (first seen wins). */
static void	mk_push(t_vec *v, const char *w, size_t n)
{
	char	*dup;
	size_t	i;

	i = 0;
	while (i < v->len)
	{
		dup = ((char **)v->ctx)[i++];
		if (ft_strlen(dup) == n && !ft_strncmp(dup, w, n))
			return ;
	}
	dup = ft_strndup((char *)w, n);
	if (dup)
		vec_push(v, &dup);
}

/* Offer w[0..n) if it passes the name filter. */
void	mk_add(t_vec *v, const char *w, size_t n)
{
	if (mk_ok(w, n))
		mk_push(v, w, n);
}
