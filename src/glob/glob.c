/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"
#include "shell.h"
#include "expander.h"
#include <dirent.h>
#include <stdlib.h>

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

/*
** Get path for opendir (use "." for empty path)
*/
char	*get_curr_path(char *path)
{
	if (*path)
		return (path);
	return (".");
}

/*
** Process a single directory entry
** Returns 1 to continue, 0 to stop
*/
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
		get_next_path(&next_path, matcher.path, diren->d_name);
		if (matcher.glob.len > res)
		{
			char ch = '/';
			vec_push(&next_path, &ch);
			if (vec_ensure_space_n(&next_path, 1))
				((char *)next_path.ctx)[next_path.len] = '\0';
			match_dir(matcher.args, matcher.glob, (char *)next_path.ctx,
				res + 1);
			free(next_path.ctx);
		}
		else
		{
			char *s = ft_strdup((char *)next_path.ctx);
			vec_push(matcher.args, &s);
			free(next_path.ctx);
		}
		return (1);
	}
	free(next_path.ctx);
	return (1);
}

/*
** Match directory entries against glob pattern
*/
void	match_dir(t_vec *args, t_vec_glob glob, char *path, size_t offset)
{
	DIR				*dir;
	t_dir_matcher	matcher;

	dir = opendir(get_curr_path(path));
	if (glob.len <= offset && dir)
	{
		char *s = ft_strdup(path);
		vec_push(args, &s);
		closedir(dir);
		return ;
	}
	if (!dir)
		return ;
	matcher = (t_dir_matcher){
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
** Helper to free string elements in a vector
*/
static void	free_str_elem(void *el)
{
	free(*(char **)el);
}

/*
** Expand a word node with glob matching
** Returns vector of matched filenames (or original word if no matches)
*/
t_vec	expand_word_glob(t_ast_node word)
{
	t_vec		args;
	t_vec_glob	glob;
	t_string	tmp;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	glob = word_to_glob(word);
	if (glob.len == 0)
	{
		char *s = ft_strdup("");
		vec_push(&args, &s);
		return (args);
	}
	if (((t_glob *)glob.ctx)[0].ty == G_SLASH)
		match_dir(&args, glob, "/", 1);
	else
		match_dir(&args, glob, "", 0);
	if (args.len == 0)
	{
		tmp = word_to_string(word);
		char *s = (char *)tmp.ctx;
		vec_push(&args, &s);
	}
	glob_free_tokens(&glob);
	if (!get_g_sig()->should_unwind)
		ft_quicksort(&args);
	if (get_g_sig()->should_unwind)
		vec_destroy(&args, free_str_elem);
	return (args);
}