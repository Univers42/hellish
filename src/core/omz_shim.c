/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   omz_shim.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"

/* oh-my-zsh, loaded from inside hellish -- issue #114.
**
** Every 42 account logs into zsh, and a great many ~/.zshrc files are the
** oh-my-zsh template: `export ZSH=~/.oh-my-zsh`, `plugins=(git ...)`,
** `source $ZSH/oh-my-zsh.sh`.  The installer's `--zshrc` import loads
** ~/.zshrc inside hellish in the zsh dialect so the aliases and functions
** come along (it was the default for three releases, which is how #114
** was filed) -- and oh-my-zsh.sh opens with a guard that refuses any
** shell whose $ZSH_VERSION is empty: it prints "Oh My Zsh can't be loaded
** from: hellish", a process tree, and returns 1.  So the one line every
** such rc has produced a screen of red at every shell start, and the
** plugins it named never loaded.  The same line typed into ~/.hellishrc
** by hand met the same guard.
**
** Setting ZSH_VERSION would be a lie with consequences: the file goes on
** to load the completion system and every lib/ *.zsh, most of which need
** zle or compsys, and the plugin corpus pins what each plugin may claim.
** What hellish CAN do is what that corpus already proves: source the
** plugins.  So `source .../oh-my-zsh.sh` -- recognised by the file's
** name, wherever $ZSH points -- runs this instead:
**
**   1. $ZSH_CUSTOM/plugins/NAME/NAME.plugin.zsh, else $ZSH/plugins/...,
**      for each NAME in $plugins, in order, in the zsh dialect;
**   2. $ZSH_CUSTOM/ *.zsh, lexically, as oh-my-zsh.sh does after them.
**
** Plugins that ARE the line editor (autosuggestions, syntax highlighting)
** are skipped by name: nothing in them can work without ZLE and every
** line of them would say so.  Themes are not loaded -- the prompt is
** hellish's own (`prompt` lists 29) and an oh-my-zsh theme is written
** against lib/ functions that need zsh -- and lib/ is not loaded for the
** same reason.  Nothing is printed: the user did not write any of these
** lines, and "not supported" once per plugin is exactly the noise this
** replaces (state->zunsup_quiet).
*/

/* $ZSH is the directory the file lives in; $ZSH_CUSTOM defaults to its
   custom/ as in oh-my-zsh.sh.  $ZSH is set when the rc did not, since
   plugins read it. */
static void	omz_dirs(t_shell *state, const char *path, t_omz *o)
{
	const char	*slash;
	char		*e;

	slash = ft_strrchr((char *)path, '/');
	if (slash)
		o->zsh = ft_substr(path, 0, (size_t)(slash - path));
	else
		o->zsh = ft_strdup(".");
	e = env_expand(state, "ZSH");
	if (!e || !*e)
		env_set(&state->env, env_create(ft_strdup("ZSH"),
				ft_strdup(o->zsh), false));
	e = env_expand(state, "ZSH_CUSTOM");
	if (e && *e)
		o->custom = ft_strdup(e);
	else
		o->custom = path_join(o->zsh, "custom");
}

/* One plugin by name: found under custom/ first, then $ZSH, like
   oh-my-zsh's is_plugin.  Unknown names are as silent as skipped ones. */
static void	omz_plugin(t_shell *state, t_omz *o, const char *name)
{
	char	*file;

	if (!*name || omz_zle_only(name))
		return ;
	file = omz_plugin_file(o, name);
	if (!file)
		return ;
	omz_source(state, file);
	xfree(file);
}

/* `plugins=git` or `plugins="git z"` -- a scalar, split on blanks. */
static void	omz_plugins_words(t_shell *state, t_omz *o, const char *val)
{
	char	**w;
	size_t	i;

	w = ft_split((char *)val, ' ');
	if (!w)
		return ;
	i = 0;
	while (w[i])
	{
		omz_plugin(state, o, w[i]);
		xfree(w[i++]);
	}
	xfree(w);
}

/* Every name in $plugins, in order.  An array is flattened to its
   blank-joined form first -- plugin names never contain a blank -- so
   one loop serves both spellings; and it is a COPY, because sourcing a
   plugin grows the environment the array text lives in. */
static void	omz_plugins(t_shell *state, t_omz *o)
{
	t_env	*e;
	char	*val;

	e = env_get(&state->env, "plugins");
	if (!e || !e->value)
		return ;
	if (arr_is(e->value))
		val = arr_join(e->value, ' ');
	else
		val = ft_strdup(e->value);
	if (val)
		omz_plugins_words(state, o, val);
	xfree(val);
}

/* The whole of oh-my-zsh.sh that can run here, in the zsh dialect and
   with the unsupported-builtin notes off for the duration. */
int	omz_shim(t_shell *state, const char *path)
{
	t_omz	o;
	bool	was;
	bool	quiet;

	omz_dirs(state, path, &o);
	was = zsh_mode_swap(state, true);
	quiet = state->zunsup_quiet;
	state->zunsup_quiet = true;
	omz_plugins(state, &o);
	omz_custom(state, &o);
	state->zunsup_quiet = quiet;
	zsh_mode_swap(state, was);
	xfree(o.zsh);
	xfree(o.custom);
	return (0);
}
