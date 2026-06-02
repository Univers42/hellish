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

static int	type_is_builtin(const char *name)
{
	return (builtin_func((char *)name) != NULL);
}

/* POSIX reserved words (plus the common function/{/}). */
static int	type_is_keyword(const char *name)
{
	static const char	*kw[] = {"!", "{", "}", "case", "do", "done", "elif",
		"else", "esac", "fi", "for", "if", "in", "then", "until", "while", NULL};
	int					i;

	i = 0;
	while (kw[i])
		if (!ft_strcmp(kw[i++], name))
			return (1);
	return (0);
}

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

static int	type_one(t_shell *state, const char *name)
{
	char	*path;
	char	*alias_val;
	char	*cached;

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
	{
		ft_printf("%s is a shell builtin\n", name);
		return (0);
	}
	cached = cmd_hash_lookup(&state->cmd_cache, name);
	if (cached)
	{
		ft_printf("%s is hashed (%s)\n", name, cached);
		return (0);
	}
	path = NULL;
	if (ft_strchr(name, '/'))
	{
		if (access(name, X_OK) == 0)
		{
			ft_printf("%s is %s\n", name, name);
			return (0);
		}
	}
	else if (type_find_in_path(state, name, &path))
	{
		ft_printf("%s is %s\n", name, path);
		free(path);
		return (0);
	}
	ft_eprintf("%s: type: %s: not found\n", state->ctx, name);
	return (1);
}

int	builtin_type(t_shell *state, t_vec argv)
{
	size_t	i;
	int		status;

	if (argv.len < 2)
		return (0);
	status = 0;
	i = 1;
	while (i < argv.len)
	{
		if (type_one(state, ((char **)argv.ctx)[i]))
			status = 1;
		i++;
	}
	return (status);
}
