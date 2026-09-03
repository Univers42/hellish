/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rc_source.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"

char	*read_file(const char *path);
int		exec_file_string(t_shell *state, char *str, const char *src);

/* Interactive startup only: load the configuration. The `metinp != INP_RL`
   guard is the part that matters: -c, scripts and piped input must NOT read
   it, or every test run would silently inherit your dotfile and you'd chase
   ghosts for an afternoon. Absent files or no $HOME? Just shrug and carry on.
**
** Order (rc_load.c owns the directories, this owns ~/.hellishrc):
**
**     /etc/hellish/rc.d/          system-wide
**     $XDG_CONFIG_HOME/hellish/rc.d/
**     $XDG_CONFIG_HOME/hellish/plugins/<name>/plugin.hsh
**     ~/.hellishrc                so your own file wins over the above
**     $XDG_CONFIG_HOME/hellish/after.d/    the imported ~/.zshrc (#114)
**
** ~/.hellishrc after the drop-ins is the compatibility promise: it is the
** file people already have, and a loader that let a dropped-in plugin
** override it is a loader people switch off.  after.d comes later still
** for the same reason one level up: it holds the user's zsh config, and
** that has the last word over the plugin framework living in ~/.hellishrc
** (rc_load.c, rc_load_after, says what broke when it did not). */

/* ~/.hellishrc is a sourced file in every sense: a parse error names it
   and its line (exec_file_string), and `return` ends it the way it ends
   ~/.bashrc instead of "can only `return' from a function". */
static void	source_rc_file(t_shell *state, const char *home)
{
	char	*path;
	char	*content;

	path = ft_strjoin(home, "/.hellishrc");
	if (!path)
		return ;
	content = read_file(path);
	if (!content)
		return (xfree(path));
	frame_push(state, NULL, path);
	state->source_depth++;
	exec_file_string(state, content, path);
	state->source_depth--;
	frame_pop(state);
	xfree(path);
	xfree(content);
}

void	source_hellishrc(t_shell *state)
{
	char	*home;

	if (state->metinp != INP_RL)
		return ;
	home = env_expand(state, "HOME");
	rc_load_all(state, home);
	if (state->option_flags & OPT_FLAG_NORC || state->rcfile)
		return ;
	if (!home || !*home)
		return ;
	source_rc_file(state, home);
	rc_load_after(state, home);
}
