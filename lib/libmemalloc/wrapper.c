/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   wrapper.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 19:30:10 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:41:48 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"

void	*arena_malloc(size_t nbytes)
{
	void	*p;

	p = malloc(nbytes);
	return (check_null(p));
}

void	*arena_realloc(void *p, size_t nbytes)
{
	p = realloc(p, nbytes);
	return (check_null(p));
}

void	arena_free(void *p)
{
	free(p);
}
