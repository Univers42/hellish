/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   check.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 20:33:18 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:36:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"
#include <stdio.h>
#include <string.h>

void	out_of_space(void)
{
	fputs("Out of space\n", stderr);
	abort();
}

void	*check_null(void *p)
{
	if (!p)
		out_of_space();
	return (p);
}

char	*str_save(const char *s)
{
	char	*d;
	size_t	n;

	if (!s)
		return (NULL);
	n = strlen(s) + 1;
	d = (char *)check_null(malloc(n));
	memcpy(d, s, n);
	return (d);
}
