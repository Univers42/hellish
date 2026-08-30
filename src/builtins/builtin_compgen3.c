/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compgen3.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"
#include <dirent.h>
#include <sys/stat.h>

/* The filesystem and alias sources for compgen, plus its option scanner. */

/* -a: the alias table, walked the same way alias_print_all walks it and
   sorted -- a hash table has no order worth exposing. */
int	cg_aliases(t_shell *st, const char *pfx)
{
	size_t			i;
	t_hash_entry	*e;
	t_vec			out;

	out = (t_vec){0};
	e = (t_hash_entry *)st->aliases.ctx;
	i = 0;
	while (e && i < st->aliases.cap)
	{
		if (e[i].key)
			cg_add(&out, e[i].key, pfx);
		i++;
	}
	return (cg_flush(&out));
}

/* Split a prefix into the directory to read and the leading bytes an entry
   must carry. `pfx` is a path fragment, so everything up to the last '/'
   is a real directory and the tail is what we filter on. */
static char	*cg_split_dir(const char *pfx, const char **tail)
{
	const char	*slash;

	slash = ft_strrchr((char *)pfx, '/');
	if (!slash)
	{
		*tail = pfx;
		return (ft_strdup("."));
	}
	*tail = slash + 1;
	if (slash == pfx)
		return (ft_strdup("/"));
	return (ft_strndup((char *)pfx, (size_t)(slash - pfx)));
}

/* Print one directory entry as a completion: the caller's prefix path plus
   the name, so the answer can be pasted back on the command line. -d keeps
   only directories; -c keeps only what is executable. */
static int	cg_entry(const char *pfx, const char *nm, char act, char *dir)
{
	char	*full;
	char	*shown;
	int		keep;

	full = ft_asprintf("%s/%s", dir, nm);
	keep = 1;
	if (act == 'd')
		keep = (cg_is_dir(full) != 0);
	else if (act == 'c')
		keep = (access(full, X_OK) == 0);
	xfree(full);
	if (!keep)
		return (0);
	shown = cg_join_prefix(pfx, nm);
	ft_printf("%s\n", shown);
	return (xfree(shown), 1);
}

/* -f / -d / -c against a path prefix: read the directory the prefix names
   and offer every entry that continues it. A hidden file is offered only
   when the prefix asks for the dot, which is what a user typing `.ba<TAB>`
   means and what stops a bare TAB from listing every dotfile. */
int	cg_glob_paths(char act, const char *pfx)
{
	DIR				*d;
	struct dirent	*de;
	const char		*tail;
	char			*dir;
	int				hit;

	dir = cg_split_dir(pfx, &tail);
	d = opendir(dir);
	if (!d)
		return (xfree(dir), 0);
	hit = 0;
	de = readdir(d);
	while (de)
	{
		if (ft_strncmp(de->d_name, (char *)tail, ft_strlen((char *)tail)) == 0
			&& (tail[0] == '.' || de->d_name[0] != '.'))
			hit += cg_entry(pfx, de->d_name, act, dir);
		de = readdir(d);
	}
	return (closedir(d), xfree(dir), hit);
}
