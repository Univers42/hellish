/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_dir.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:12:42 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 05:07:43 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/* Handle a directory entry that matched the current pattern segment. If more
   pattern tokens remain (glob.len > res), the match is partial -- the entry
   must be a directory and we recurse into it. Otherwise the match is complete
   and we push the full path string onto `args`. The path string is built by
   get_next_path and freed after the push or recursive call. */
static void	handle_glob_match_result(t_dir_matcher matcher,
										t_string *next_path,
										struct dirent *diren,
										size_t res)
{
	get_next_path(next_path, matcher.path, diren->d_name);
	if (matcher.glob.len > res)
	{
		vec_push_char(next_path, '/');
		if (vec_ensure_space_n(next_path, 1))
			((char *)next_path->ctx)[next_path->len] = '\0';
		match_dir(matcher.args, matcher.glob,
			(char *)next_path->ctx, res + 1);
		xfree(next_path->ctx);
	}
	else
	{
		vec_push(matcher.args, &(char *)
		{ft_strdup((char *)next_path->ctx)});
		xfree(next_path->ctx);
	}
}

/* Skip '.' and '..'; readdir always returns them and they would cause infinite
   recursion or nonsense paths if we tried to glob into them. */
static bool	is_dot_or_dotdot(const char *name)
{
	return (name[0] == '.' && (name[1] == '\0'
			|| (name[1] == '.' && name[2] == '\0')));
}

/* Read and process one directory entry. Returns 1 to continue iterating,
   0 when readdir returns NULL (end of directory). '.' and '..' are always
   skipped. For each remaining entry, matches_pattern decides whether it
   fits the current pattern segment; a match triggers handle_glob_match_result
   which either recurses or collects the path. Non-matching entries free the
   (empty) next_path allocation and continue. */
int	process_dir(t_dir_matcher matcher)
{
	struct dirent	*diren;
	t_string		next_path;
	size_t			res;

	diren = readdir(matcher.dir);
	if (!diren)
		return (0);
	if (is_dot_or_dotdot(diren->d_name))
		return (1);
	res = matches_pattern(diren->d_name, matcher.glob, matcher.offset, true);
	vec_init(&next_path);
	next_path.elem_size = 1;
	if (res)
	{
		handle_glob_match_result(matcher, &next_path, diren, res);
		return (1);
	}
	return (xfree(next_path.ctx), 1);
}

/* Recursively walk a directory tree matching the glob token array segment by
   segment. `path` is the filesystem path opened so far; `offset` is the index
   into `glob` of the first token that still needs to be matched. An empty
   `path` is treated as "." for opendir (POSIX: empty string is not a valid
   path). If glob is already exhausted when we open the directory, `path`
   itself is added to args (handles patterns ending in '/'). The should_unwind
   flag breaks out early if a signal arrived -- we don't want to spend time
   finishing a glob after Ctrl-C. */
void	match_dir(t_vec *args, t_vec_glob glob, char *path, size_t offset)
{
	DIR				*dir;
	t_dir_matcher	matcher;
	char			*s;

	dir = opendir(get_curr_path(path));
	if (!dir)
		return ;
	if (glob.len <= offset)
	{
		s = ft_strdup(path);
		vec_push(args, &s);
	}
	else if (!handle_globstar(args, glob, path, offset)
		&& !handle_dot_segment(args, glob, path, offset))
	{
		matcher = (t_dir_matcher){.path = path, .dir = dir,
			.glob = glob, .offset = offset, .args = args};
		while (!get_g_sig()->should_unwind && process_dir(matcher))
			;
	}
	closedir(dir);
}

/* Build the full path for a directory entry by appending `fname` to `path`.
   The result is NUL-terminated via vec_ensure_space_n. The caller owns the
   buffer and must free it. This avoids snprintf/PATH_MAX for arbitrary-length
   paths since the vec grows dynamically. */
void	get_next_path(t_string *next_path, char *path, char *fname)
{
	vec_init(next_path);
	next_path->elem_size = 1;
	vec_push_str(next_path, path);
	vec_push_str(next_path, fname);
	if (!vec_ensure_space_n(next_path, 1))
		return ;
	((char *)next_path->ctx)[next_path->len] = '\0';
}
