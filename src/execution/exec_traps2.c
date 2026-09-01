/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_traps2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* DEBUG / RETURN / ERR are not inherited by called functions (bash without
   functrace / errtrace): on entry the caller's three traps are lifted out
   and the slots blanked, so the body starts with a clean slate; on exit
   the function-scoped RETURN trap has already fired. A trap the body SET,
   though, is a global assignment in bash and must survive the return —
   the old "free the body's, put the caller's back" here silently
   discarded it (issue #96: a preexec hook installed from a function never
   fired, with no diagnostic). Measured against bash 5.3.9: a body-set
   trap wins over the saved one; a slot the body left (or reset to) empty
   gets the caller's trap back. */

/* Save the three pseudo-trap slots into `save[0..2]` and blank them. */
void	trap_save_reset(t_shell *state, char **save)
{
	save[0] = state->traps[TRAP_DEBUG];
	save[1] = state->traps[TRAP_RETURN];
	save[2] = state->traps[TRAP_ERR];
	state->traps[TRAP_DEBUG] = NULL;
	state->traps[TRAP_RETURN] = NULL;
	state->traps[TRAP_ERR] = NULL;
}

/* Keep a trap the body set (freeing the caller's), restore the caller's
   where the body set none. */
static void	trap_restore_one(t_shell *state, int slot, char *saved)
{
	if (state->traps[slot])
		xfree(saved);
	else
		state->traps[slot] = saved;
}

void	trap_restore(t_shell *state, char **save)
{
	trap_restore_one(state, TRAP_DEBUG, save[0]);
	trap_restore_one(state, TRAP_RETURN, save[1]);
	trap_restore_one(state, TRAP_ERR, save[2]);
}

/* Called on the child side of every subshell-environment fork (command
   substitution, ( ), coproc): the inherited DEBUG/RETURN/ERR traps stay
   listable but stop firing, as bash does. set_one_trap clears the bit for
   a slot the child assigns, so its own traps fire. */
void	pseudo_traps_quiet(t_shell *state)
{
	state->traps_quiet = (1 << (TRAP_DEBUG - TRAP_DEBUG))
		| (1 << (TRAP_RETURN - TRAP_DEBUG))
		| (1 << (TRAP_ERR - TRAP_DEBUG));
}
