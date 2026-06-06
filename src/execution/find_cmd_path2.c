/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   find_cmd_path2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:10:20 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:53:46 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "cmd_hash.h"
#include "ft_builtins.h"

/* A matching file was found in PATH but it lacks execute permission.  Set
   errno=EACCES so err_1_errno prints the right strerror, free the shell
   state (we are inside a forked child), and return EXE_PERM_DENIED (126)
   to propagate the correct exit code up through find_exe_path_wrapper. */
int	handle_perm_denied(t_shell *state, char *cmd_name)
{
	errno = EACCES;
	err_1_errno(state, cmd_name);
	free_all_state(state);
	return (EXE_PERM_DENIED);
}

/* Match bash's command hashing: when a bare name (no '/', not a builtin or
   function) is about to run as an external, resolve it through PATH in the
   PARENT and cache the hit, so a later `type`/`hash` reports it as hashed.
   The resolution is quiet -- on a miss we cache nothing and the forked child
   still prints "command not found". cmd_hash_insert copies the path, so we
   free our buffer afterwards. */
void	prehash_external(t_shell *state, char *argv0)
{
	char	**dirs;
	char	*path;
	int		denied;

	if (!argv0 || ft_strchr(argv0, '/') || builtin_func(argv0)
		|| func_lookup(state, argv0)
		|| cmd_hash_lookup(&state->cmd_cache, argv0)
		|| !env_expand(state, "PATH"))
		return ;
	dirs = ft_split(env_expand(state, "PATH"), ':');
	denied = 0;
	path = exe_path(dirs, argv0, &denied);
	if (path)
		cmd_hash_insert(&state->cmd_cache, argv0, path);
	free_tab(dirs);
	xfree(path);
}
