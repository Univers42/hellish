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

/* Is the byte in hand the line delimiter, unescaped? Never under -N, where
   the delimiter is ordinary data and only the count ends the read. */
bool	rd_at_delim(char ch, t_rdopt *o, bool bs)
{
	if (o->exact)
		return (false);
	return (ch == o->delim && !(bs && !o->raw));
}
