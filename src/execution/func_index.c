/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   func_index.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 13:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/27 13:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

/* A name -> slot index over state->functions.
**
** WHY. func_lookup() was a linear scan, and store_function() calls it before
** every insert, so DEFINING n functions was O(n^2) and CALLING one was O(n).
** Measured against bash --norc, 2000 calls of one function:
**
**     functions defined      hellish      bash
**              0              3.6 ms      6.8 ms     <- hellish 2x FASTER
**            500             20.1 ms      7.1 ms
**           2000             74.3 ms     10.2 ms
**           5000            208.0 ms     13.2 ms     <- 15.8x SLOWER
**
** hellish's dispatch is genuinely faster than bash's; the scan is what threw
** that away. It only mattered once configs got big -- which is exactly what a
** plugin system is, so this had to be fixed before rc.d/plugins ship or the
** loader would get the blame.
**
** WHY INDICES AND NOT POINTERS. state->functions is a t_vec of structs, so
** vec_push reallocs and every t_shell_func* into it dangles. The hash stores
** slot+1 as the value (0 is indistinguishable from "absent" in hash_get), and
** callers turn that back into a live pointer through vec_idx.
**
** unset_function compacts the vec with a memmove, which shifts every slot
** after the hole, so the index is rebuilt there rather than patched. That is
** O(n) -- the same order as the memmove it follows -- and unset is rare. */

void	func_index_init(t_shell *state)
{
	hash_init(&state->func_index, 64);
}

/* Point `name` at slot `idx`. hash_set strdups the key and hash_destroy
   frees its own copies, so the caller's string need not outlive the call --
   and freeing t_shell_func.name elsewhere is not a double free. */
void	func_index_set(t_shell *state, const char *name, size_t idx)
{
	hash_set(&state->func_index, name, (void *)(uintptr_t)(idx + 1));
}

/* Rebuild from scratch. Used after unset_function's memmove, and cheap
   enough there that patching the shifted range is not worth the subtlety. */
void	func_index_rebuild(t_shell *state)
{
	t_shell_func	*arr;
	size_t			i;

	hash_destroy(&state->func_index, NULL);
	hash_init(&state->func_index, 64);
	arr = (t_shell_func *)state->functions.ctx;
	i = 0;
	while (i < state->functions.len)
	{
		func_index_set(state, arr[i].name, i);
		i++;
	}
}

t_shell_func	*func_index_get(t_shell *state, const char *name)
{
	uintptr_t	slot;

	slot = (uintptr_t)hash_get(&state->func_index, name);
	if (!slot || slot - 1 >= state->functions.len)
		return (NULL);
	return ((t_shell_func *)vec_idx(&state->functions, slot - 1));
}
