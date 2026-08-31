/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_star2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include <sys/stat.h>

/* `shopt -s globstar` -- `**` crossing directory levels.
**
** The option was stored and did nothing: the tokenizer collapsed every star
** run to one G_ASTERISK, so `gs/ ** /f` matched exactly `gs/<one segment>/f`
** and `shopt -s globstar` reported `on` while changing no behaviour.  An
** option that answers "on" and is not is worse than a missing one -- a
** script that checks for it gets a yes and then quietly searches one level.
**
** `**` is not a pattern that matches slashes; it is a WALK.  So it is
** implemented where the walk lives, as two independent descents from each
** directory:
**
**   1. zero levels  -- go on with the rest of the pattern right here
**   2. one level    -- for every subdirectory, retry the SAME `**` inside it
**
** which between them enumerate every depth exactly once.  A trailing `**`
** with no pattern after it matches everything below, this directory
** included, which is why `gs2/ **` yields `gs2/` itself first.
*/

/* Is `path` a directory we may descend into?  A symlink is deliberately not
   followed: `**` on a tree with a link back to its own root would otherwise
   never terminate, and bash does not follow them here either. */
static bool	gs_is_dir(const char *path)
{
	struct stat	st;

	if (lstat(path, &st) != 0)
		return (false);
	return (S_ISDIR(st.st_mode));
}

/* Path + name + '/', NUL-terminated, ready to hand back to match_dir. */
static char	*gs_child_path(const char *path, const char *name, bool slash)
{
	t_string	p;

	vec_init(&p);
	p.elem_size = 1;
	vec_push_str(&p, (char *)path);
	vec_push_str(&p, (char *)name);
	if (slash)
		vec_push_char(&p, '/');
	vec_push_char(&p, '\0');
	return ((char *)p.ctx);
}

static void	gs_descend(t_vec *args, t_vec_glob glob, char *path);

/* One directory entry during a `**` descent: emit it when the pattern ends
   here (a trailing `**` matches every path below), then continue into it
   when it is a directory -- the rest of the pattern applied at this depth,
   and the globstar itself applied one level further down.
     It recurses into gs_descend rather than back through match_dir on
   purpose.  Going through match_dir would re-enter handle_globstar, whose
   trailing-`**` step emits the directory ITSELF -- which this function has
   already emitted, without the trailing slash.  That produced every
   directory twice, as `gs2/a` and `gs2/a/`.
     Dotfiles follow the same rule as every other segment -- hidden unless
   dotglob -- so `**` does not silently walk into .git. */
static void	gs_entry(t_vec *args, t_vec_glob glob, char *path, const char *nm)
{
	char	*child;

	if (nm[0] == '.' && !glob_dotglob())
		return ;
	if (finished_pattern(glob, 1))
	{
		child = gs_child_path(path, nm, false);
		vec_push(args, &child);
	}
	child = gs_child_path(path, nm, true);
	if (gs_is_dir(child))
	{
		if (glob.len > 1)
			match_dir(args, glob, child, 2);
		gs_descend(args, glob, child);
	}
	xfree(child);
}

/* Read `path` and run gs_entry over every entry except . and .. */
static void	gs_descend(t_vec *args, t_vec_glob glob, char *path)
{
	DIR				*dir;
	struct dirent	*de;

	dir = opendir(get_curr_path(path));
	if (!dir)
		return ;
	de = readdir(dir);
	while (de && !get_g_sig()->should_unwind)
	{
		if (!(de->d_name[0] == '.' && (de->d_name[1] == '\0'
					|| (de->d_name[1] == '.' && de->d_name[2] == '\0'))))
			gs_entry(args, glob, path, de->d_name);
		de = readdir(dir);
	}
	closedir(dir);
}

/* The `**` segment at `offset`, if there is one.  True when it was handled,
   so match_dir can skip its ordinary readdir scan.  The two descents are
   independent and both run: zero levels (the rest of the pattern, applied
   right here) and one level (the same `**`, applied one directory down).
     `glob` is re-based to the globstar token so the recursion always sees it
   at index 0, which is what lets gs_entry ask `finished_pattern(glob, 1)`
   without carrying an offset through every frame.
     A TRAILING `**` takes the zero-level step over the globstar alone, which
   makes match_dir emit the directory itself -- `gtest/ **` lists `gtest/`
   before its contents.  Only when there is a prefix: a bare `**` in the cwd
   has no name to emit, and bash prints no leading empty field there. */
bool	handle_globstar(t_vec *args, t_vec_glob glob, char *path, size_t off)
{
	t_vec_glob	rest;

	if (off >= glob.len
		|| ((t_glob *)vec_idx(&glob, off))->ty != G_GLOBSTAR)
		return (false);
	rest = glob;
	rest.ctx = (t_glob *)glob.ctx + off;
	rest.len = glob.len - off;
	if (rest.len > 1)
		match_dir(args, rest, path, 2);
	else if (*path)
		match_dir(args, rest, path, 1);
	gs_descend(args, rest, path);
	return (true);
}
