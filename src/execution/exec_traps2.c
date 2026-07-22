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
   the function-scoped RETURN trap has already fired and any traps the body
   set are freed before the caller's are put back. */

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

/* Free any traps the function body set, then restore the caller's. */
void	trap_restore(t_shell *state, char **save)
{
	xfree(state->traps[TRAP_DEBUG]);
	xfree(state->traps[TRAP_RETURN]);
	xfree(state->traps[TRAP_ERR]);
	state->traps[TRAP_DEBUG] = save[0];
	state->traps[TRAP_RETURN] = save[1];
	state->traps[TRAP_ERR] = save[2];
}
