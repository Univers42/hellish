/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:03 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:03 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_builtins.h"
#include "shell.h"
#include "helpers.h"
#include "env.h"
#include "sh_input.h"
#include "job_control.h"
#include "update.h"
#include "sys.h"
#include <stdlib.h>
#include <fcntl.h>
#include <locale.h>
#include <unistd.h>

void		on(t_shell *state, char **argv, char **envp);
void		run_pending_traps(t_shell *state);
void		run_exit_trap(t_shell *state);
char		*env_expand(t_shell *state, char *key);
int			setup_output_buffer(t_shell *state, int *bak);
void		flush_output_buffer(int buf_fd, int bak);
static void	repl_shell(t_shell *state);
static void	off(t_shell *state);
void		source_hellishrc(t_shell *state);
void		source_profile(t_shell *state);
void		set_default_ps1(t_shell *state);

/* Entry point. A login shell is started with argv[0] beginning with '-' (the
   old getty convention -- "-bash", "-hellish"); we strip that dash so $0 looks
   normal everywhere else, but we remember it: the flag has to be raised AFTER
   on() has parsed argv, or cli parsing would clear what we just set, and it is
   OR-ed in so an explicit --login still counts. Notice there is no explicit
   return: off() already forwards the last command's status as our exit code,
   so just falling off the end of main() with that set is exactly the
   behaviour we want. */
int	main(int argc, char **argv, char **envp)
{
	t_shell	state;
	bool	is_login_shell;

	is_login_shell = (argv[0] && argv[0][0] == '-');
	if (is_login_shell)
		argv[0]++;
	(void)argc;
	setlocale(LC_ALL, "");
	on(&state, argv, envp);
	tty_snapshot_save();
	if (is_login_shell)
		state.option_flags |= OPT_FLAG_LOGIN;
	set_default_ps1(&state);
	source_profile(&state);
	source_hellishrc(&state);
	maybe_spawn_update_check(&state);
	show_welcome(&state);
	repl_shell(&state);
	off(&state);
}

/* Open one REPL turn: hand out a fresh input buffer and clear the "Ctrl-C
   just unwound us" flag, then run $PROMPT_COMMAND in the current shell
   right before each interactive primary prompt (bash behaviour).
   Non-interactive shells never touch it. The last command's status is
   preserved so the prompt's $? badge reflects the user's command, not
   PROMPT_COMMAND's.

   The two exit flags are also aged here, and that is what makes "ask me
   twice" mean twice IN A ROW. exit_warned used to be cleared only when a
   BUILTIN ran, so an external command in between left it set -- warn once,
   run `ps`, press Ctrl-D, and the shell walked out over a stopped job it
   had already been told about (issue #58). Carrying exit_attempt forward
   for exactly one turn expresses bash's actual rule: the warning stands
   only while the previous turn was itself an attempt to leave. Anything
   else you do re-arms it. */
static void	open_cycle(t_shell *state)
{
	t_execution_state	saved;
	char				*pc;

	vec_init(&state->input);
	state->input.elem_size = 1;
	state->rl.eof_refused = false;
	state->cmd_no++;
	state->exit_warned = state->exit_attempt;
	state->exit_attempt = false;
	get_g_sig()->should_unwind = 0;
	update_notify_prompt(state);
	if (state->metinp != INP_RL)
		return ;
	tty_snapshot_refresh();
	pc = env_expand(state, "PROMPT_COMMAND");
	if (pc && *pc)
	{
		saved = state->last_cmd_st_exe;
		exec_string(state, pc);
		set_cmd_status(state, saved);
	}
	run_hook_funcs(state, "HELLISH_PRECMD_FUNCS", NULL);
	run_zsh_prompt_hooks(state, "precmd", NULL);
}

/* The read-eval-print loop -- the beating heart of the shell. Each turn hands
   out a fresh input buffer, clears the "Ctrl-C just unwound us" flag, reports
   any background jobs that finished, then parses and runs exactly one command.
   The pile of frees at the bottom is the whole trick to staying leak-flat: the
   AST, redirects, input and heredoc scratch are per-command, so we drop them
   each turn. A script can run for an hour and memory just stays flat. */
static void	repl_shell(t_shell *state)
{
	int	buf_fd;
	int	stdout_bak;

	buf_fd = setup_output_buffer(state, &stdout_bak);
	while (!state->should_exit)
	{
		open_cycle(state);
		job_notify(state);
		parse_and_execute_input(state);
		run_pending_traps(state);
		free_redirects(&state->redirects);
		free_ast(&state->tree);
		xfree(state->input.ctx);
		state->input = (t_string){0};
		if (state->alias_exp_owned)
			xfree(state->alias_exp.ctx);
		state->alias_exp = (t_string){0};
		state->alias_exp_owned = false;
		xfree(state->hd_src);
		state->hd_src = NULL;
		state->hd_pos = 0;
		xfree(state->hd_stripped);
		state->hd_stripped = NULL;
	}
	flush_output_buffer(buf_fd, stdout_bak);
}

/* On the way out: fire the EXIT trap first (bash runs it last, right before
   the shell dies), then free the environment and everything else we own, and
   finally exit carrying the last command's status. Order matters -- the trap
   may still want to read variables, so we free the env *after* it runs.
   The status is snapshotted *before* the trap runs: POSIX says the shell
   exits with the status it had on reaching the trap, not with the status of
   the trap body (`trap 'echo bye' EXIT; false` exits 1, and a script dying
   on a parse error still exits 2 after its trap).  Only an explicit `exit N`
   inside the body overrides that, by never returning here at all. */
static void	off(t_shell *state)
{
	t_execution_state	final;

	final = state->last_cmd_st_exe;
	run_exit_trap(state);
	jobs_hangup_on_exit(state);
	tty_snapshot_restore();
	free_env(&state->env);
	free_all_state(state);
	forward_exit_status(final);
}
