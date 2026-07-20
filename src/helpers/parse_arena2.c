/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_arena2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/20 19:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/20 19:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parena.h"
#include "ft_memory.h"

/* Gate the arena: true only around the main input cycle's parse_tokens
   call. Nested parses (eval/source/cmdsub) run with the gate closed and
   allocate from the general heap, keeping their per-call free discipline. */
void	parena_on(bool on)
{
	parena()->on = on;
}

/* End-of-cycle reclaim: everything the parse allocated dies here in O(1)
   per chunk. The first chunk is kept warm so command-per-line scripts do
   not re-allocate 256KB every cycle; the rest is returned to the heap so a
   one-off giant script does not pin its high-water mark forever. */
void	parena_reset(void)
{
	t_parena	*a;
	int			i;

	a = parena();
	i = 1;
	while (i < a->n_chunks)
	{
		xfree(a->chunk[i]);
		a->chunk[i] = NULL;
		a->size[i] = 0;
		i++;
	}
	if (a->n_chunks > 1)
		a->n_chunks = 1;
	a->cur = 0;
	a->off = 0;
}

/* Session teardown (original process only, from free_all_state): return
   every chunk including the warm one. */
void	parena_destroy(void)
{
	t_parena	*a;
	int			i;

	a = parena();
	i = 0;
	while (i < a->n_chunks)
	{
		xfree(a->chunk[i]);
		a->chunk[i] = NULL;
		i++;
	}
	a->n_chunks = 0;
	a->cur = 0;
	a->off = 0;
	a->on = false;
}
