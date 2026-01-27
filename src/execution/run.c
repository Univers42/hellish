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

/* returns status */
int	actually_run(t_shell *state, t_vec *args)
{
	char	*path_of_exe;
	char	**envp;
	int		status;

	path_of_exe = NULL;
	ft_assert(args->len >= 1);
	run_builtin_or_continue(state, args);
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
