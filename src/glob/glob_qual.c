/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_qual.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 11:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 11:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include "ft_glob.h"
#include <sys/stat.h>

/* zsh GLOB QUALIFIERS -- the `(DNY2)` in
**
**     content=("${extract_dir}"/[*](DNY2))     <- a star, bracketed here so
**                                              this comment does not nest
**
** which is oh-my-zsh's extract asking "give me at most two entries here,
** dotfiles included, and nothing if the directory is empty".
**
** A POST-FILTER, deliberately, rather than a change to the walk. The walk
** already knows how to find names; a qualifier only ever narrows what it
** found. Threading them into match_dir would put filesystem-type questions
** inside a pattern matcher that has none today, for no gain -- the
** directories a plugin globs hold tens of entries, not millions.
**
** Implemented: D dotfiles, N nullglob, . plain files, / directories,
** @ symlinks, Y<n> stop after n. That is what the corpus uses and no more;
** an unknown qualifier is a SYNTAX ERROR rather than an ignored character,
** because ignoring `om` (sort by mtime) would hand back the same files in
** the wrong order and nothing would say so.
**
** Gated on the zsh dialect. In bash `*(N)` is an extglob pattern meaning
** "zero or one N", which is a different language for the same bytes.
*/

/* Does `path` pass the type qualifiers?  With none of . / @ set, everything
   passes -- they are a whitelist, and an empty whitelist means no filter
   rather than nothing allowed.
     lstat, NOT stat: a symlink to a regular file is a SYMLINK, so `(.)`
   excludes it and `(@)` is what selects it. Following the link would make
   `(.)` and `(@)` both true for the same entry, which is not a partition
   and not what zsh reports. (zsh spells "follow the link" as `(-.)`, a
   modifier this does not implement.) */
static bool	gq_type_ok(const char *path, t_gqual *q)
{
	struct stat	st;

	if (!q->plain && !q->dir && !q->link)
		return (true);
	if (lstat(path, &st) != 0)
		return (false);
	if (q->link && S_ISLNK(st.st_mode))
		return (true);
	if (q->plain && S_ISREG(st.st_mode))
		return (true);
	if (q->dir && S_ISDIR(st.st_mode))
		return (true);
	return (false);
}

/* A dotfile is one whose LAST path component starts with a dot. Checking the
   whole path would drop everything under a hidden directory, which is not
   what (D) is about -- it is about this entry's own name. */
static bool	gq_is_dot(const char *path)
{
	const char	*base;

	base = ft_strrchr((char *)path, '/');
	if (base)
		return (base[1] == '.');
	return (path[0] == '.');
}

/* Drop one element, freeing it and closing the gap. */
static void	gq_erase(t_vec *v, size_t at)
{
	char	**a;

	a = (char **)v->ctx;
	xfree(a[at]);
	while (at + 1 < v->len)
	{
		a[at] = a[at + 1];
		at++;
	}
	v->len--;
}

/* Apply the qualifiers to a finished match list, in place.
     (D) is not applied here: the WALK decides whether to offer dotfiles at
   all, because a dotfile the walk skipped is not in the list to keep. What
   is left for this pass is the reverse -- without (D), drop any dotfile the
   walk did offer, which is how `*(N)` stays free of them while `*(DN)` does
   not. */
void	glob_qual_apply(t_vec *args, t_gqual *q)
{
	size_t	i;

	if (!q->on)
		return ;
	i = 0;
	while (i < args->len)
	{
		if ((!q->dots && gq_is_dot(((char **)args->ctx)[i]))
			|| !gq_type_ok(((char **)args->ctx)[i], q))
			gq_erase(args, i);
		else
			i++;
	}
	while (q->limit > 0 && args->len > (size_t)q->limit)
		gq_erase(args, args->len - 1);
}
