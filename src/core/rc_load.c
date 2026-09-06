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
int		exec_file_string(t_shell *state, char *content, const char *src);

/* The configuration load path (issue #70).
**
** ~/.hellishrc was the only entry point, which is why every tool that wanted
** to add something to a hellish session had to append to it -- and why the
** `# >>> managed by ... >>>` marker convention had to exist at all. There was
** nowhere else to put anything.
**
**     /etc/hellish/rc.d/ *.hsh *.zsh          system-wide, lexical order
**     $XDG_CONFIG_HOME/hellish/rc.d/ *.hsh *.zsh    yours, lexical order
**     $XDG_CONFIG_HOME/hellish/plugins/ * /plugin.hsh
**     ~/.hellishrc                            so it wins over the above
**     $XDG_CONFIG_HOME/hellish/after.d/ *.hsh *.zsh  the late slot, below
**
** Lexical order is why the convention is 10-env, 20-aliases, 30-prompt: it is
** the one ordering rule a user can see from `ls` without reading any code.
**
** A .zsh module is read in the zsh dialect -- the rule `source` already
** applies to the extension (zsh_path) -- and sorted in with the .hsh ones by
** name. That is the marker-free home for a zsh-flavoured config: the rc a
** 42 student pastes from ~/.zshrc (issue #112) needs no `emulate zsh` line
** when it is saved as rc.d/50-mine.zsh. The dialect is restored when the
** file ends, exactly as for a sourced plugin.
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
	char	*zero;

	content = read_file(path);
	if (!content)
		return ;
	frame_push(state, NULL, path);
	if (zsh_path(path))
		zsh_mode_swap(state, true);
	zero = zsh_zero_bind(state, path);
	state->source_depth++;
	exec_file_string(state, content, path);
	state->source_depth--;
	zsh_zero_restore(state, zero);
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
	{
		collect(dir, ".hsh", files);
		collect(dir, ".zsh", files);
	}
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

/* The late slot: after.d, sourced AFTER ~/.hellishrc (issue #114).
**
** It exists for one file, the `install.sh --zshrc` opt-in import of
** ~/.zshrc (after.d/90-zshrc.zsh), and the reason is ordering.  The
** plugin framework lives in ~/.hellishrc and its git plugin defines
** gwip() as a FUNCTION; oh-my-zsh's git plugin, which the import loads,
** defines gwip as an ALIAS.  Alias first, function second is a syntax
** error in every shell -- the alias expands inside the definition -- and
** rc.d loads first, so the import there broke the framework at every
** start.  A user's own zsh config is the last word in zsh (~/.zshrc IS
** the last file read), so it is the last word here too: the alias simply
** wins, and nothing errors.  Same rules as rc.d otherwise: .hsh and .zsh,
** lexical order, --norc and --rcfile skip it.
**
** Why an opt-in and not the default: a .zshrc is code written for zsh.
** One began with `exec /bin/bash` and hellish replaced itself with bash
** at every start (#116); one ran promptinit and compinit (#115).  The
** shell does not run another shell's rc unless asked, by name. */
static void	source_dir(t_shell *state, const char *dir)
{
	t_vec	files;
	size_t	i;

	vec_init(&files);
	files.elem_size = sizeof(char *);
	collect(dir, ".hsh", &files);
	collect(dir, ".zsh", &files);
	sort_strvec(&files, 0);
	i = 0;
	while (i < files.len)
	{
		source_one(state, ((char **)files.ctx)[i]);
		xfree(((char **)files.ctx)[i++]);
	}
	xfree(files.ctx);
}

void	rc_load_after(t_shell *state, const char *home)
{
	char	*xdg;
	char	*dir;

	if (state->option_flags & OPT_FLAG_NORC || state->rcfile)
		return ;
	xdg = xdg_config_hellish(state, home);
	if (!xdg)
		return ;
	dir = path_join(xdg, "after.d");
	if (dir)
		source_dir(state, dir);
	xfree(dir);
	xfree(xdg);
}
