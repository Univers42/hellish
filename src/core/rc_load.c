/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rc_load.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 12:50:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 12:50:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "env.h"
#include "ft_builtins.h"
#include <dirent.h>
#include <sys/stat.h>

char	*read_file(const char *path);
int		exec_string(t_shell *state, char *content);

/* The configuration load path (issue #70).
**
** ~/.hellishrc was the only entry point, which is why every tool that wanted
** to add something to a hellish session had to append to it -- and why the
** `# >>> managed by ... >>>` marker convention had to exist at all. There was
** nowhere else to put anything.
**
**     /etc/hellish/rc.d/ *.hsh                system-wide, lexical order
**     $XDG_CONFIG_HOME/hellish/rc.d/ *.hsh    yours, lexical order
**     $XDG_CONFIG_HOME/hellish/plugins/ * /plugin.hsh
**     ~/.hellishrc                            LAST, so it always wins
**
** Lexical order is why the convention is 10-env, 20-aliases, 30-prompt: it is
** the one ordering rule a user can see from `ls` without reading any code.
**
** ~/.hellishrc keeping the last slot is deliberate. It is the file people
** already have and already edit; a loader that let a dropped-in plugin
** override it would be a loader people turn off.
**
** Each file is sourced with a frame pushed, so ${BASH_SOURCE[0]} inside it
** names the file itself -- that is what lets a plugin find its own directory
** and load siblings, rather than hardcoding a path. */

static void	source_one(t_shell *state, const char *path)
{
	char	*content;

	content = read_file(path);
	if (!content)
		return ;
	frame_push(state, NULL, path);
	if (zsh_path(path))
		zsh_mode_swap(state, true);
	exec_string(state, content);
	frame_pop(state);
	xfree(content);
}

/* Build the ordered file list for one root. */
static void	add_root(t_shell *state, const char *root, t_vec *files)
{
	char	*dir;
	size_t	first;

	(void)state;
	dir = path_join(root, "rc.d");
	first = files->len;
	if (dir)
		collect(dir, ".hsh", files);
	xfree(dir);
	sort_strvec(files, first);
	dir = path_join(root, "plugins");
	first = files->len;
	if (dir)
		collect_plugins(dir, files);
	xfree(dir);
	sort_strvec(files, first);
}

/* Source everything, in order. --norc skips the lot; --rcfile=F replaces it
   with exactly F, which is what makes the suites able to pin a known config
   instead of inheriting the developer's. */
void	rc_load_all(t_shell *state, const char *home)
{
	t_vec	files;
	char	*xdg;
	size_t	i;

	if (state->option_flags & OPT_FLAG_NORC)
		return ;
	if (state->rcfile)
		return (source_one(state, state->rcfile), (void)0);
	vec_init(&files);
	files.elem_size = sizeof(char *);
	add_root(state, "/etc/hellish", &files);
	xdg = xdg_config_hellish(state, home);
	if (xdg)
		add_root(state, xdg, &files);
	xfree(xdg);
	i = 0;
	while (i < files.len)
	{
		source_one(state, ((char **)files.ctx)[i]);
		xfree(((char **)files.ctx)[i++]);
	}
	xfree(files.ctx);
}
