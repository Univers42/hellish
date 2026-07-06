/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_dir2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include <unistd.h>

/* A pattern segment that is literally `.` or `..` can never match a
   readdir entry — process_dir skips those names to avoid infinite
   recursion — yet POSIX wants explicit dot segments to work: a pattern
   like "/<star>/.." must expand to every /dir/.. the way bash does.
   These helpers peek at the segment and, when it is exactly one literal
   dot or dot-dot token (followed by a slash or the end of the pattern),
   splice it into the path verbatim instead of scanning the directory. */

static bool	seg_is_dots(t_vec_glob *glob, size_t offset)
{
	t_glob	*g;

	g = (t_glob *)vec_idx(glob, offset);
	if (g->ty != G_LITERAL)
		return (false);
	if (!(g->len == 1 && g->start[0] == '.')
		&& !(g->len == 2 && g->start[0] == '.' && g->start[1] == '.'))
		return (false);
	if (offset + 1 < glob->len
		&& ((t_glob *)vec_idx(glob, offset + 1))->ty != G_SLASH)
		return (false);
	return (true);
}

/* Recurse past the dot segment's slash, or emit the finished path if the
   dot segment ends the pattern and the path really exists. */
static void	dot_recurse_or_emit(t_vec *args, t_vec_glob glob,
				t_string *next, size_t offset)
{
	char	*s;

	if (offset + 1 < glob.len)
	{
		vec_push_char(next, '/');
		if (!vec_ensure_space_n(next, 1))
			return ;
		((char *)next->ctx)[next->len] = '\0';
		match_dir(args, glob, (char *)next->ctx, offset + 2);
		return ;
	}
	if (!vec_ensure_space_n(next, 1))
		return ;
	((char *)next->ctx)[next->len] = '\0';
	if (access((char *)next->ctx, F_OK) == 0)
	{
		s = ft_strdup((char *)next->ctx);
		vec_push(args, &s);
	}
}

/* Entry point called by match_dir before the readdir scan.  Returns true
   when the segment at `offset` was a literal dot segment and has been
   fully handled (path built, recursion or emission done). */
bool	handle_dot_segment(t_vec *args, t_vec_glob glob, char *path,
			size_t offset)
{
	t_string	next;
	t_glob		*g;

	if (!seg_is_dots(&glob, offset))
		return (false);
	g = (t_glob *)vec_idx(&glob, offset);
	vec_init(&next);
	next.elem_size = 1;
	vec_push_str(&next, path);
	vec_push_nstr(&next, (char *)g->start, g->len);
	dot_recurse_or_emit(args, glob, &next, offset);
	return (xfree(next.ctx), true);
}
