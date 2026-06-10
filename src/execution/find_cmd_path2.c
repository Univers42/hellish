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

/* Keep the split-$PATH cache AND the command-location hash in sync with
   the current $PATH.  POSIX (XCU 2.9.1.1): changing PATH must make the
   shell forget remembered command locations — bash flushes its hash on
   every PATH assignment.  We detect the change lazily by comparing the
   exact current PATH string against the one the caches were built from
   (validate-on-use: no env_set hook to forget, staleness impossible by
   construction).  On change: flush cmd_cache, rebuild the split.  The
   first-ever sync skips the flush — nothing was hashed under another
   PATH yet. */
void	path_cache_sync(t_shell *state)
{
	char	*cur;

	cur = env_expand(state, "PATH");
	if (cur && state->path_dirs_src
		&& ft_strcmp(cur, state->path_dirs_src) == 0)
		return ;
	if (state->path_dirs_src)
		cmd_hash_clear(&state->cmd_cache);
	if (state->path_dirs)
		free_tab(state->path_dirs);
	xfree(state->path_dirs_src);
	state->path_dirs = NULL;
	state->path_dirs_src = NULL;
	if (!cur)
		return ;
	state->path_dirs_src = ft_strdup(cur);
	state->path_dirs = ft_split(cur, ':');
}

/* Match bash's command hashing: when a bare name (no '/', not a builtin or
   function) is about to run as an external, resolve it through PATH in the
   PARENT and cache the hit, so a later `type`/`hash` reports it as hashed.
   The resolution is quiet -- on a miss we cache nothing and the forked child
   still prints "command not found". cmd_hash_insert copies the path, so we
   free our buffer afterwards. The sync must run BEFORE the hash lookup (a
   PATH change flushes the hash); the cached split replaces the per-call
   ft_split + free_tab this function used to pay. */
void	prehash_external(t_shell *state, char *argv0)
{
	char	*path;
	int		denied;

	if (!argv0 || ft_strchr(argv0, '/') || builtin_func(argv0)
		|| func_lookup(state, argv0))
		return ;
	path_cache_sync(state);
	if (cmd_hash_lookup(&state->cmd_cache, argv0) || !state->path_dirs)
		return ;
	denied = 0;
	path = exe_path(state->path_dirs, argv0, &denied);
	if (path)
		cmd_hash_insert(&state->cmd_cache, argv0, path);
	xfree(path);
}
