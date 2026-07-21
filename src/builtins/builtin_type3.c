/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_type3.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_alias.h"
#include "cmd_hash.h"

/* type -t NAME: print a single word category — alias, keyword, function,
   builtin, file — or nothing (status 1) when unknown. Priority matches
   bash and the plain `type` form. Used constantly in scripts to branch
   on "is this a builtin or a real command", so it must be byte-exact. */
int	type_t_one(t_shell *state, const char *name)
{
	char	*path;

	if (alias_get(&state->aliases, name))
		return (ft_printf("alias\n"), 0);
	if (type_is_keyword(name))
		return (ft_printf("keyword\n"), 0);
	if (func_lookup(state, (char *)name))
		return (ft_printf("function\n"), 0);
	if (type_is_builtin(name))
		return (ft_printf("builtin\n"), 0);
	if (cmd_hash_lookup(&state->cmd_cache, name))
		return (ft_printf("file\n"), 0);
	if (ft_strchr(name, '/') && access(name, X_OK) == 0)
		return (ft_printf("file\n"), 0);
	if (type_find_in_path(state, name, &path))
		return (ft_printf("file\n"), xfree(path), 0);
	return (1);
}

/* type -p NAME: print the resolved path IF the name is an external file
   (nothing for builtins/keywords/functions); type -P forces the PATH
   search even when a builtin/keyword would shadow it. */
int	type_p_one(t_shell *state, const char *name, int force)
{
	char	*path;

	if (!force && (type_is_keyword(name) || func_lookup(state, (char *)name)
			|| type_is_builtin(name)))
		return (0);
	if (ft_strchr(name, '/') && access(name, X_OK) == 0)
		return (ft_printf("%s\n", name), 0);
	if (type_find_in_path(state, name, &path))
		return (ft_printf("%s\n", path), xfree(path), 0);
	return (1);
}

/* Dispatch one name under the active -t / -p / -P mode ('t','p','P', or
   0 for the default long form). */
int	type_dispatch(t_shell *state, const char *name, char mode)
{
	if (mode == 't')
		return (type_t_one(state, name));
	if (mode == 'p')
		return (type_p_one(state, name, 0));
	if (mode == 'P')
		return (type_p_one(state, name, 1));
	return (type_one(state, name));
}
