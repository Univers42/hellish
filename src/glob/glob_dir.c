/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_dir.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:12:42 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 11:35:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/*
** Process a single directory entry
** Returns 1 to continue, 0 to stop
*/
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
		free(next_path->ctx);
	}
	else
	{
		vec_push(matcher.args, &(char *)
		{ft_strdup((char *)next_path->ctx)});
		free(next_path->ctx);
	}
}

int	process_dir(t_dir_matcher matcher)
{
	struct dirent	*diren;
	t_string		next_path;
	size_t			res;

	diren = readdir(matcher.dir);
	if (!diren)
		return (0);
	res = matches_pattern(diren->d_name, matcher.glob, matcher.offset, true);
	vec_init(&next_path);
	next_path.elem_size = 1;
	if (res)
	{
		handle_glob_match_result(matcher, &next_path, diren, res);
		return (1);
	}
	return (free(next_path.ctx), 1);
}

/*
** Match directory entries against glob pattern
*/
void	match_dir(t_vec *args, t_vec_glob glob, char *path, size_t offset)
{
	DIR				*dir;
	t_dir_matcher	matcher;
	char			*s;

	dir = opendir(get_curr_path(path));
	if (glob.len <= offset && dir)
	{
		s = ft_strdup(path);
		vec_push(args, &s);
		closedir(dir);
		return ;
	}
	if (!dir)
		return ;
	matcher = (t_dir_matcher)
	{
		.path = path,
		.dir = dir,
		.glob = glob,
		.offset = offset,
		.args = args
	};
	while (!get_g_sig()->should_unwind && process_dir(matcher))
		;
	closedir(dir);
}

/*
** Build next path by appending filename to current path
*/
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
