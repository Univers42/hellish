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

/* Is element `key` of an ORDINARY parameter set?  The fallback for a base
   that is not one of zsh's special tables -- an associative array a script
   built itself, or a table we do not have.

   `${+terminfo[kcub1]}` is the second kind, and 0 is the RIGHT answer for
   it rather than a shrug: terminfo is a loadable module, real zsh without
   that module also answers 0, and the plugin's `if` is written to be false
   in exactly that case.  What must not happen is answering 0 for a table we
   DO have and simply failed to consult -- which is why the special three
   are checked first and this is the fallback, not the other way round. */
int	zp_elem_set(t_shell *state, const char *base, int blen, char *key)
{
	char	*val;
	char	*elem;

	val = env_expand_n(state, (char *)base, blen);
	if (!val)
		return (0);
	if (assoc_is(val))
		elem = assoc_get(val, key, (int)ft_strlen(key));
	else if (arr_is(val))
		elem = arr_get_idx(val, sub_to_index(state, ft_atol(key),
					arr_count(val)));
	else
		return (sub_to_index(state, ft_atol(key), 1) == 0);
	if (!elem)
		return (0);
	return (xfree(elem), 1);
}
