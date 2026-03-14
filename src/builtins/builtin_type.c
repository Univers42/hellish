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

static int	type_is_builtin(const char *name)
{
	return (builtin_func((char *)name) != NULL);
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

	if (type_is_builtin(name))
	{
		ft_printf("%s is a shell builtin\n", name);
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
