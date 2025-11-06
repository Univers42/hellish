/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   st_all.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 19:32:10 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:44:10 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"
#include <string.h>

char	*st_nputs(const char *s, size_t n, char *p)
{
	p = make_str_space(n, p);
	memcpy(p, s, n);
	return (p + n);
}

char	*st_puts(const char *s, char *p)
{
	if (s)
		return (st_nputs(s, (size_t)strlen(s), p));
	return (st_nputs(s, 0, p));
}

char	*st_unputc(char *p)
{
	if (p)
		--p;
	return (p);
}

char	*st_adjust(ptrdiff_t amount, char *p)
{
	return (p + amount);
}

char	*st_putc(int c, char *p)
{
	t_garena	*g;

	g = arena_ctx();
	if (p == g->sstrend)
		p = grow_stack_str();
	*p++ = (char)c;
	return (p);
}
