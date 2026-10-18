/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:12:09 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

/* Resolve argv[0] on PATH and execve it, consulting NEITHER the function
   table nor the builtin table.  Publishes the resolved path as ULTIMATE_ARG
   ("_"), builds envp, then execve -- which only returns on failure, so
   everything after try_exec_with_fallback is the diagnostic, cleanup and
   the status.
   Runs in a forked child; it must never return to the parent's call stack.
     Split out of actually_run for `command`: skipping the function lookup
   is the entire point of that builtin, and it needs this half alone. */
int	exec_external_argv(t_shell *state, t_vec *args)
{
	char	*path_of_exe;
	char	**envp;
	int		status;
	int		err;

	path_of_exe = NULL;
	ft_assert(args->len >= 1);
	status = try_cmd_not_found_handler(state, args);
	if (status >= 0)
		return (status);
	status = find_exe_path_wrapper(state,
			((char **)(args->ctx))[0], &path_of_exe);
	if (status != 0)
		return (status);
	env_set(&state->env,
		env_create(ft_strdup(ULTIMATE_ARG), ft_strdup(path_of_exe), true));
	envp = get_envp(state, path_of_exe);
	err = try_exec_with_fallback(path_of_exe, args, envp);
	status = exec_failure_report(state, path_of_exe, err);
	cleanup_after_exec_failure(args, path_of_exe, envp);
	return (status);
}

/* The child side of executing an external command.  Builtins and functions
   win first (run_builtin_or_continue calls exit()); everything else falls
   through to the plain external path. */
int	actually_run(t_shell *state, t_vec *args)
{
	ft_assert(args->len >= 1);
	run_builtin_or_continue(state, args);
	return (exec_external_argv(state, args));
}
