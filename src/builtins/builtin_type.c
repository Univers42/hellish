/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_type.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_alias.h"
#include "cmd_hash.h"

/* Check via the dispatch hash table — no need to maintain a separate list.
   The dispatch table IS the authoritative set of builtins. */
static int	type_is_builtin(const char *name)
{
	return (builtin_func((char *)name) != NULL);
}

/* POSIX reserved words (plus the common function/{/}). */
static int	type_is_keyword(const char *name)
{
	static const char	*kw[] = {"!", "{", "}", "case", "do", "done",
		"elif", "else", "esac", "fi", "for", "if", "in", "then",
		"until", "while", NULL};
	int					i;

	i = 0;
	while (kw[i])
		if (!ft_strcmp(kw[i++], name))
			return (1);
	return (0);
}

/* Search $PATH for `name`. Returns 1 and sets *out to an allocated path on
   success; returns 0 on failure. Caller must xfree *out. We deliberately
   skip the command cache here so `type` always does a fresh search (the
   cache might be stale after a PATH change). */
static int	type_find_in_path(t_shell *state, const char *name, char **out)
{
	char	*path;
	char	**dirs;
	int		perm;

	path = env_expand(state, "PATH");
	if (!path)
		return (0);
	dirs = ft_split(path, ':');
	if (!dirs)
		return (0);
	perm = 0;
	*out = exe_path(dirs, (char *)name, &perm);
	free_tab(dirs);
	return (*out != NULL);
}

/* PATH resolution for type. Check the command cache first (the "hashed"
   form). Then, if the name contains a slash, stat it directly. Otherwise
   do a full PATH search. Reports "not found" on stderr and returns 1. */
int	type_one_path(t_shell *state, const char *name)
{
	char	*path;
	char	*cached;

	cached = cmd_hash_lookup(&state->cmd_cache, name);
	if (cached)
		return (ft_printf("%s is hashed (%s)\n", name, cached), 0);
	path = NULL;
	if (ft_strchr(name, '/'))
	{
		if (access(name, X_OK) == 0)
			return (ft_printf("%s is %s\n", name, name), 0);
	}
	else if (type_find_in_path(state, name, &path))
	{
		ft_printf("%s is %s\n", name, path);
		xfree(path);
		return (0);
	}
	ft_eprintf("%s: type: %s: not found\n", state->ctx, name);
	return (1);
}

/* Classify one name: alias > keyword > function > builtin > external.
   The priority matches bash exactly — an alias shadows everything, a
   function shadows a builtin, etc. Each path prints one line and returns. */
int	type_one(t_shell *state, const char *name)
{
	char	*alias_val;

	alias_val = alias_get(&state->aliases, name);
	if (alias_val)
	{
		ft_printf("%s is aliased to `%s'\n", name, alias_val);
		return (0);
	}
	if (type_is_keyword(name))
		return (ft_printf("%s is a shell keyword\n", name), 0);
	if (func_lookup(state, (char *)name))
		return (ft_printf("%s is a function\n", name), 0);
	if (type_is_builtin(name))
		return (ft_printf("%s is a shell builtin\n", name), 0);
	return (type_one_path(state, name));
}
