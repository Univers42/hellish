/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd_helpers5.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

/* True if `path` names an existing directory (used to validate a CDPATH
   candidate before we commit to it). */
static bool	cd_is_dir(const char *path)
{
	struct stat	st;

	if (stat(path, &st) != 0)
		return (false);
	return (S_ISDIR(st.st_mode));
}

/* CDPATH is only consulted for plain names: an absolute operand or one that
   starts with "./" or "../" (or is "." / "..") is taken literally. */
static bool	cd_skip_cdpath(const char *op)
{
	if (op[0] == '/')
		return (true);
	if (op[0] == '.' && (op[1] == '\0' || op[1] == '/'))
		return (true);
	if (op[0] == '.' && op[1] == '.' && (op[2] == '\0' || op[2] == '/'))
		return (true);
	return (false);
}

/* Build one candidate path from a CDPATH component and the operand. An empty
   component means "current directory", so the operand is used unprefixed. */
static char	*cd_cdpath_one(const char *comp, const char *op)
{
	char	*pre;
	char	*res;

	if (comp[0] == '\0')
		return (ft_strdup(op));
	pre = ft_strjoin(comp, "/");
	if (!pre)
		return (NULL);
	res = ft_strjoin(pre, op);
	return (xfree(pre), res);
}

/* Search $CDPATH for `op`, returning the first component under which it exists
   as a directory (caller frees), or NULL when CDPATH does not apply / misses.
   A hit via a non-empty component sets *echo so the destination is printed,
   matching bash. */
char	*cd_cdpath(t_shell *state, char *op, bool *echo)
{
	char	*cdpath;
	char	**comps;
	char	*cand;
	int		i;

	if (cd_skip_cdpath(op))
		return (NULL);
	cdpath = env_expand(state, "CDPATH");
	if (!cdpath || !cdpath[0])
		return (NULL);
	comps = ft_split(cdpath, ':');
	if (!comps)
		return (NULL);
	i = -1;
	while (comps[++i])
	{
		cand = cd_cdpath_one(comps[i], op);
		if (cand && cd_is_dir(cand))
			return (*echo = (comps[i][0] != '\0'), free_tab(comps), cand);
		xfree(cand);
	}
	return (free_tab(comps), NULL);
}
