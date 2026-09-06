/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read6.c                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <stdlib.h>

/* Is the byte in hand the line delimiter, unescaped? Never under -N, where
   the delimiter is ordinary data and only the count ends the read. */
bool	rd_at_delim(char ch, t_rdopt *o, bool bs)
{
	if (o->exact)
		return (false);
	return (ch == o->delim && !(bs && !o->raw));
}

/* Feed one byte to the character counter and return the count so far: a
   complete character, or an invalid byte (which bash counts as one), bumps
   it; a partial sequence does not. */
long	rd_count(t_rdcount *c, char ch)
{
	size_t	r;

	if (MB_CUR_MAX == 1)
		return (++c->chars);
	r = mbrtowc(NULL, &ch, 1, &c->st);
	if (r == (size_t)-2)
		return (c->chars);
	if (r == (size_t)-1)
		ft_memset(&c->st, 0, sizeof(c->st));
	return (++c->chars);
}
