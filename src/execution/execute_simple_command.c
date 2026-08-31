/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_command.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:52 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:52 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

void	exit_clean(t_shell *state, int code);

void	replace_null_argv_with_empty(t_executable_cmd *cmd);

/* Run a shell function in the parent process.  Pre-assignment NAME=val
   words are applied temporarily around the call (POSIX: they must be
   visible inside the function body) then undone via restore_temp_assigns
   when it returns.  Redirects are saved/restored with prep_redir so the
   function's stderr/stdout don't bleed back into the caller.  The argv
   and redirect resources belong to the cmd struct and are freed last.
     `fn` is passed in rather than looked up from argv[0] because
   command_not_found_handle runs through this same path with a name that is
   deliberately NOT argv[0] -- one call site for "run a function here",
   so the redirect and temp-assign handling cannot diverge between them. */
static t_execution_state	handle_func_call(t_shell *state,
						t_executable_cmd *cmd,
						t_executable_node *exe,
						t_shell_func *fn)
{
	t_execution_state	res;
	int					bak[3];
	int					need;
	t_vec				saves;

	need = prep_redir(state, exe, bak, 0);
	procsub_close_fds_parent(state);
	saves = apply_temp_assigns(state, &cmd->pre_assigns);
	res = execute_func_call(state, fn, &cmd->argv);
	restore_temp_assigns(state, &saves);
	if (need)
		restore_backup_fds(bak, 0);
	free_executable_cmd(state, *cmd);
	free_executable_node(exe);
	return (res);
}

/* A command whose argv[0] expanded to an empty string is not a valid
   command name.  We emit the "command not found" diagnostic using the
   expansion context (state->ctx) and return 127, the POSIX exit code for
   a command that was not found. */
static t_execution_state	handle_empty_command(t_shell *state,
						t_executable_cmd *cmd,
						t_executable_node *exe)
{
	ft_eprintf("%s: command not found\n", state->ctx);
	procsub_close_fds_parent(state);
	free_executable_cmd(state, *cmd);
	free_executable_node(exe);
	return (res_status(COMMAND_NOT_FOUND));
}

/* No command word, only NAME=val assignments.  POSIX: when run in the
   parent context these assignments become permanent environment variables;
   inside a subshell or pipeline child they'd normally be discarded, but
   modify_parent_ctx is false there so we skip env_extend.  The last
   command-substitution status ($?) is preserved so `x=$(cmd)` sets $?
   correctly even though there is no foreground command here. */
static t_execution_state	handle_assign_only(t_shell *state,
								t_executable_cmd *cmd,
								t_executable_node *exe)
{
	if (exe->modify_parent_ctx)
		env_extend(&state->env, &cmd->pre_assigns, state->opt_allexport);
	procsub_close_fds_parent(state);
	free_executable_cmd(state, *cmd);
	free_executable_node(exe);
	return (res_status(state->last_cmdsub_status));
}

/* Route the expanded command to its handler.  Priority order matters:
   - empty argv    -> assign_only (NAME=val list with no command)
   - argv[0]==""   -> error (expansion produced empty string)
   - function name -> handle_func_call IN PARENT (functions can cd, set
     variables, etc. -- they MUST run in parent, never in a fork)
   - builtin name  -> execute_builtin_cmd_fg IN PARENT (same reason: cd,
     export, read etc. modify parent state)
   - not found     -> command_not_found_handle, if the user defined it
   - anything else -> execute_cmd_bg which forks and calls execve.
   The modify_parent_ctx guard for functions and builtins ensures that
   when they appear as non-last pipeline stages they still fork (the
   pipeline executor cleared modify_parent_ctx for those).
     The not-found hook is DECIDED here and RUN in a fork (cnf_fork.c):
   the message it replaces is printed by the exec child, so the choice must
   be made before forking, while bash gives the handler a subshell -- it
   carries no modify_parent_ctx guard because it never runs in the parent. */
static t_execution_state	dispatch_cmd(t_shell *state,
								t_executable_cmd *cmd,
								t_executable_node *exe)
{
	char	*argv0;

	if (cmd->argv.len == 0 || !cmd->argv.ctx)
		return (handle_assign_only(state, cmd, exe));
	argv0 = ((char **)cmd->argv.ctx)[0];
	if (argv0 && argv0[0] == '\0')
		return (handle_empty_command(state, cmd, exe));
	if (state->functions.len && func_lookup(state, argv0)
		&& exe->modify_parent_ctx)
		return (handle_func_call(state, cmd, exe, func_lookup(state, argv0)));
	if (builtin_func(argv0) && exe->modify_parent_ctx)
		return (execute_builtin_cmd_fg(state, cmd, exe));
	prehash_external(state, argv0);
	return (execute_cmd_bg(state, exe, cmd));
}

/* The full lifecycle of a simple command: expand its AST word nodes into
   a concrete argv + pre-assign list (expand_simple_command), sanitise any
   NULL pointers that can appear after glob/IFS expansion, optionally
   print the trace (set -x), then dispatch (aliases were already spliced
   by the input scanner before the lexer ran).  An expansion error
   (ambiguous redirect, signal unwind) short-circuits before dispatch.
   The cmd struct is always freed on all paths to stay leak-flat. */
t_execution_state	execute_simple_command(t_shell *state,
									t_executable_node *exe)
{
	t_executable_cmd	cmd;
	bool				fatal;

	cmd = (t_executable_cmd){0};
	state->last_cmdsub_status = 0;
	note_cmd_lineno(state, exe->node);
	fire_debug_trap(state);
	if (expand_simple_command(state, exe->node, &cmd, &exe->redirs))
	{
		fatal = redir_err_is_fatal(state, &cmd);
		procsub_close_fds_parent(state);
		free_executable_cmd(state, cmd);
		free_executable_node(exe);
		if (get_g_sig()->should_unwind)
			return (res_status(CANCELED));
		if (fatal)
			exit_clean(state, 1);
		return (res_status(AMBIGUOUS_REDIRECT));
	}
	if (!cmd.argv.ctx)
		cmd.argv.len = 0;
	replace_null_argv_with_empty(&cmd);
	if (state->opt_xtrace && cmd.argv.len > 0)
		xtrace_print(state, &cmd.argv);
	return (dispatch_cmd(state, &cmd, exe));
}
