/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_traps.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* The non-signal traps DEBUG / RETURN / ERR run shell code inline during
   the tree walk (unlike signal traps, which queue and fire between
   commands). Each is gated on its slot being set and on trap_depth, so a
   trap body — which itself runs simple commands and may fail — never
   recursively re-triggers DEBUG/ERR. $? is set to the reported code for
   the body to read ($? inside an ERR trap is the failing status) and
   restored afterwards, so the surrounding script's status is undisturbed. */

/* Run trap slot `slot` with $? presented as `code`, guarded against
   re-entry. No-op when the slot is empty (the hot path for DEBUG), and
   when the trap was inherited by a subshell environment (traps_quiet —
   bash never fires an inherited DEBUG/RETURN/ERR inside $() or ( )). */
static void	run_trap_body(t_shell *state, int slot, int code)
{
	t_execution_state	saved;

	if (!state->traps[slot] || state->trap_depth
		|| (state->traps_quiet & (1 << (slot - TRAP_DEBUG))))
		return ;
	saved = state->last_cmd_st_exe;
	set_cmd_status(state, res_status(code));
	state->trap_depth++;
	exec_string(state, state->traps[slot]);
	state->trap_depth--;
	set_cmd_status(state, saved);
}

/* Before each simple command (bash's DEBUG trap). $? stays the previous
   command's status while the body runs. */
void	fire_debug_trap(t_shell *state)
{
	run_trap_body(state, TRAP_DEBUG, state->last_cmd_st_exe.status);
}

/* After a command fails, under the same suppression rules as set -e
   (errexit_check is the sole caller). Independent of -e being on. */
void	fire_err_trap(t_shell *state, int code)
{
	run_trap_body(state, TRAP_ERR, code);
}

/* When a function returns (execute_func_call). Skipped while the shell is
   tearing down or unwinding — that path runs the EXIT trap, not RETURN. */
void	fire_return_trap(t_shell *state, int code)
{
	if (state->should_exit || get_g_sig()->should_unwind)
		return ;
	run_trap_body(state, TRAP_RETURN, code);
}
