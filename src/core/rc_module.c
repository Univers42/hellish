/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rc_module.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 17:02:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:14:50 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "helpers.h"
#include <dirent.h>
#include <unistd.h>

/* Which files in a config directory are modules (rc.d, after.d, plugins).
**
** A module may be called x.hsh, x.sh or x.zsh -- the extension says what
** it is -- or just x, with no dot at all, when its first line is a shebang
** naming polish or hellish (rc_shebang_is_ours). The shebang gate is what
** keeps README, LICENSE and an editor's `x~` backup from being sourced:
** a dotted name with any other extension never loads, and a bare name
** loads only when it says, in its first line, that this shell reads it. */
bool	rc_is_module(const char *dir, const char *name)
{
	char	*path;
	bool	ours;

	if (name[0] == '.')
		return (false);
	if (str_ends_with(name, ".hsh") || str_ends_with(name, ".sh")
		|| str_ends_with(name, ".zsh"))
		return (true);
	if (ft_strchr(name, '.') || str_ends_with(name, "~"))
		return (false);
	path = path_join(dir, name);
	ours = path && rc_shebang_is_ours(path);
	xfree(path);
	return (ours);
}

/* Every module of one directory, unsorted (the caller sorts). */
void	collect_modules(const char *dir, t_vec *out)
{
	DIR				*d;
	struct dirent	*e;
	char			*p;

	d = opendir(dir);
	if (!d)
		return ;
	e = readdir(d);
	while (e)
	{
		if (rc_is_module(dir, e->d_name))
		{
			p = path_join(dir, e->d_name);
			if (p)
				vec_push(out, &p);
		}
		e = readdir(d);
	}
	closedir(d);
}

static char	*entry_try(const char *sub, const char *name, bool need_shebang)
{
	char	*p;

	p = path_join(sub, name);
	if (p && access(p, R_OK) == 0
		&& (!need_shebang || rc_shebang_is_ours(p)))
		return (p);
	xfree(p);
	return (NULL);
}

/* A plugin directory's entry point: plugin.hsh, else plugin.sh, else a
   bare `plugin` whose shebang names this shell. First match wins, so a
   plugin that ships two never loads twice. */
char	*rc_plugin_entry(const char *sub)
{
	char	*p;

	p = entry_try(sub, "plugin.hsh", false);
	if (!p)
		p = entry_try(sub, "plugin.sh", false);
	if (!p)
		p = entry_try(sub, "plugin", true);
	return (p);
}
