/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_which.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 01:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 01:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "ft_builtins.h"
#include "cmd_hash.h"

char	*exe_path(char **path_dirs, char *exe_name, int *perm_denied);
void	path_cache_sync(t_shell *state);

/* Resolve one name the way a command lookup would: a builtin or a function
   counts, then $PATH. Returns an owned path (or a copy of the name for a
   builtin), NULL when nothing answers to it.
     Goes through the SAME cached $PATH split the executor uses rather than
   re-splitting: `(( $+commands[x] ))` shows up inside loops, and a plugin
   that probes six decompressors would otherwise split $PATH six times. */
char	*zp_which(t_shell *state, char *name)
{
	char	*path;
	int		denied;

	if (!name || !*name)
		return (NULL);
	if (builtin_func(name) || func_lookup(state, name))
		return (ft_strdup(name));
	if (ft_strchr(name, '/'))
	{
		if (access(name, X_OK) == 0)
			return (ft_strdup(name));
		return (NULL);
	}
	path_cache_sync(state);
	if (!state->path_dirs)
		return (NULL);
	denied = 0;
	path = exe_path(state->path_dirs, name, &denied);
	return (path);
}
