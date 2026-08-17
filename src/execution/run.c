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

/* Print the EACCES diagnostic and clean up after an execve failure.  We
   save and restore errno around the cleanup calls because free functions
   (xfree, free_tab) can internally call libc routines that clobber errno
   before map_errno_to_exit gets a chance to read it. */
static void	preserve_errno_exec_failed(t_shell *state, char **path_of_exe,
									t_vec *args, char **envp)
{
	int	saved_errno;

	saved_errno = errno;
	if (saved_errno == EACCES)
		ft_eprintf("%s: %s: %s\n", state->ctx,
			*path_of_exe, strerror(saved_errno));
	cleanup_after_exec_failure(args, *path_of_exe, envp);
	errno = saved_errno;
}

/* Resolve argv[0] on PATH and execve it, consulting NEITHER the function
   table nor the builtin table.  Publishes the resolved path as ULTIMATE_ARG
   ("_"), builds envp, then execve -- which only returns on failure, so
   everything after try_exec_with_fallback is cleanup + status mapping.
   Runs in a forked child; it must never return to the parent's call stack.
     Split out of actually_run for `command`: skipping the function lookup
   is the entire point of that builtin, and it needs this half alone. */
int	exec_external_argv(t_shell *state, t_vec *args)
{
	char	*path_of_exe;
	char	**envp;
	int		status;

	path_of_exe = NULL;
	ft_assert(args->len >= 1);
	status = find_exe_path_wrapper(state,
			((char **)(args->ctx))[0], &path_of_exe);
	if (status != 0)
		return (status);
	env_set(&state->env,
		env_create(ft_strdup(ULTIMATE_ARG), ft_strdup(path_of_exe), true));
	envp = get_envp(state, path_of_exe);
	try_exec_with_fallback(path_of_exe, args, envp);
	preserve_errno_exec_failed(state, &path_of_exe, args, envp);
	return (map_errno_to_exit());
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
