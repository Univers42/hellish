/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper_stack.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 19:30:35 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:37:55 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"

char	*stack_block(void)
{
	return (arena_ctx()->stack_next);
}

char	*stack_str_end(void)
{
	return (arena_ctx()->sstrend);
}

size_t	stack_block_size(void)
{
	return (arena_ctx()->stack_nleft);
}

char	*start_stack_str(void)
{
	return (stack_block());
}

char	*stack_str_nul(char *p)
{
	t_garena	*g;

	g = arena_ctx();
	if (p == g->sstrend)
		p = grow_stack_str();
	*p = '\0';
	return (p);
}
