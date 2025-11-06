/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   grab.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 19:32:38 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:37:27 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"

void	ungrab_stack_str(void *s)
{
	st_unalloc(s);
}

void	grab_stack_str(char *p)
{
	char	*base;

	base = stack_block();
	if (p >= base)
		(void)st_alloc((size_t)(p - base));
}

void	grab_stack_block(size_t len)
{
	(void)st_alloc(len);
}
