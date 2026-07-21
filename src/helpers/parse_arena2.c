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

/* Record that HEAP memory was attached to the cycle tree (arith cache,
   heredoc body, an expander token rewrite, or an arena-full fallback).
   While the flag is clear, every pointer the teardown walk would route
   through parena_free is arena-owned or NULL — so the walk is a pure
   no-op and the cycle finalize skips it entirely, reclaiming the tree
   with the O(chunks) parena_reset instead of an O(nodes) traversal.
   Missing a site can only leak (never double-free); the SAFE=1 ASan
   suite is the net that would catch such a miss. */
void	parena_note_attach(void)
{
	parena()->attached = true;
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
	a->attached = false;
}

/* In-place growth for the LAST live allocation: when `p` (rounded size
   old_n) sits exactly at the bump tip of the current chunk and the chunk
   has room for new_n, just advance the offset — no copy, no abandoned
   block. The parser builds depth-first, so the children array under
   construction nearly always owns the tip; this turns doubling-with-
   memcpy into O(1) and stops growth garbage from doubling the arena.
   The bounds test doubles as the arena-membership test: p+old == tip
   with p >= base can only hold for the tip allocation itself. */
bool	parena_try_extend(void *p, size_t old_n, size_t new_n)
{
	t_parena	*a;
	char		*base;

	a = parena();
	if (!a->on || !p || a->n_chunks == 0)
		return (false);
	old_n = PARENA_ROUND(old_n);
	new_n = PARENA_ROUND(new_n);
	base = (char *)a->chunk[a->cur];
	if ((char *)p < base || (char *)p + old_n != base + a->off)
		return (false);
	if ((size_t)((char *)p - base) + new_n > a->size[a->cur])
		return (false);
	a->off = (size_t)((char *)p - base) + new_n;
	return (true);
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
