/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rc_load_scan.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 12:50:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 12:50:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "helpers.h"
#include <dirent.h>
#include <unistd.h>

/* Directory scanning for the config load path. Split from rc_load.c
   only because the 42 norm caps a file at five functions. */

/* Collect the entries of one directory into `out`, sorted. Sorting is the
   contract, not a convenience: without it readdir order (hash order on ext4)
   decides which of 10-env and 20-aliases wins, and it differs per machine. */
void	collect(const char *dir, const char *suffix, t_vec *out)
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
		if (e->d_name[0] != '.' && str_ends_with(e->d_name, suffix))
		{
			p = path_join(dir, e->d_name);
			if (p)
				vec_push(out, &p);
		}
		e = readdir(d);
	}
	closedir(d);
}

/* One candidate: plugins/<name>/plugin.hsh, kept only if it is readable. */
static void	try_plugin(const char *dir, const char *name, t_vec *out)
{
	char	*sub;
	char	*p;

	sub = path_join(dir, name);
	if (!sub)
		return ;
	p = path_join(sub, "plugin.hsh");
	xfree(sub);
	if (p && access(p, R_OK) == 0)
		vec_push(out, &p);
	else
		xfree(p);
}

/* plugins/<name>/plugin.hsh -- a plugin is a DIRECTORY, so it can ship data
   and sibling files next to its entry point. */
void	collect_plugins(const char *dir, t_vec *out)
{
	DIR				*d;
	struct dirent	*e;

	d = opendir(dir);
	if (!d)
		return ;
	e = readdir(d);
	while (e)
	{
		if (e->d_name[0] != '.')
			try_plugin(dir, e->d_name, out);
		e = readdir(d);
	}
	closedir(d);
}
