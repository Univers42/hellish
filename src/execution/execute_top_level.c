/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:41 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:41 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include <time.h>

/* Monotonic wall clock in milliseconds, for the prompt's "took Ns"
   segment. CLOCK_MONOTONIC so a system clock change mid-command can't
   produce a negative or absurd duration. */
static long long	now_ms(void)
{
	struct timespec	ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return (0);
	return ((long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* Heredoc pre-pass for this cycle: gather_heredocs fills temp files
   before any child forks.  Top-level lists defer instead (hd_defer):
   execute_simple_list gathers each ;/&-range just before it runs, so a
   body like `$(f)` expands after the earlier commands have executed. */
static void	prepare_heredocs(t_shell *state)
{
	state->heredoc_idx = 0;
	state->hd_defer = state->cycle_has_hd
		&& (state->tree.node_type == AST_SIMPLE_LIST
			|| state->tree.node_type == AST_COMPOUND_LIST);
	if (!get_g_sig()->should_unwind && !state->hd_defer
		&& state->cycle_has_hd)
		gather_heredocs(state, &state->tree, false);
}

/* Ctrl-C interactively just needs the newline the killed foreground
   command never printed.  While running a script (non-interactive:
   metinp != INP_RL) it must set should_exit so the script aborts rather
   than stumbling on to the next command. */
static void	report_ctrl_c(t_shell *state)
{
	if (state->metinp == INP_RL)
		ft_eprintf("\n");
	else
		state->should_exit = true;
}

/* Entry point called once per parsed statement by the REPL.  It wraps the
   whole execution lifecycle for one top-level tree: pre-scan heredocs,
   execute the tree, clean up process-substitution fds, and record the
   exit status.  The verbose(CLAP_PRINT) call at the end is the
   "execution applause" hook -- a hook for debug/test output after each
   top-level command. */
void	execute_top_level(t_shell *state)
{
	t_executable_node	exe;
	t_execution_state	res;
	long long			t0;

	t0 = now_ms();
	exe = create_exe_node(0, 1, &state->tree, true);
	vec_init(&exe.redirs);
	exe.redirs.elem_size = sizeof(int);
	prepare_heredocs(state);
	if (!get_g_sig()->should_unwind)
		res = execute_tree_node(state, &exe);
	else
		res = res_status(CANCELED);
	cleanup_proc_subs(state);
	if (res.ctrl_c)
		report_ctrl_c(state);
	state->last_cmd_st_exe = res;
	state->last_cmd_ms = now_ms() - t0;
	verbose(CLAP_PRINT, "");
}
