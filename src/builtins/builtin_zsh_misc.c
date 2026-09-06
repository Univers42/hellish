/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_misc.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 20:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 20:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

int		builtin_source(t_shell *state, t_vec argv);

/* emulate [-L] {zsh|sh|ksh|bash} -- switch dialect.
**
** This is the one door that cannot itself be gated, because it is how the
** dialect gets turned on: `emulate zsh` is the first line of a great many
** plugins and the only declaration of dialect their author wrote down.
**
** -L means "local to the enclosing scope" and maps to the frame's own
** restore (see t_call_frame.zsh). WITHOUT -L the request is global, like
** every zsh option outside localoptions -- and since the rc file is
** itself sourced through a frame, "global" has to mean zsh_mode_pin: the
** unconditional per-frame restore would otherwise disarm the dialect the
** moment ~/.hellishrc finished, leaving `emulate zsh` in an rc armed for
** exactly as long as nobody was looking at it.
*/
int	builtin_emulate(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	bool	local;

	av = (char **)argv.ctx;
	i = 1;
	local = false;
	while (i < argv.len && av[i][0] == '-')
	{
		if (ft_strchr(av[i], 'L'))
			local = true;
		i++;
	}
	if (i >= argv.len)
		return (ft_eprintf("%s: emulate: not enough arguments\n",
				state->ctx), 1);
	if (!ft_strcmp(av[i], "zsh"))
		return (zsh_mode_req(state, true, local));
	if (!ft_strcmp(av[i], "sh") || !ft_strcmp(av[i], "ksh")
		|| !ft_strcmp(av[i], "bash"))
		return (zsh_mode_req(state, false, local));
	return (ft_eprintf("%s: emulate: %s: no such shell\n",
			state->ctx, av[i]), 1);
}

/* Look one autoloadable function up along $fpath / $FPATH.  Returns an owned
   path, or NULL when nothing there defines it. */
static char	*fpath_find(t_shell *state, const char *name)
{
	char	*dirs;
	char	**parts;
	char	*hit;
	int		i;

	dirs = env_expand(state, "fpath");
	if (!dirs || !*dirs)
		dirs = env_expand(state, "FPATH");
	if (!dirs || !*dirs)
		return (NULL);
	parts = ft_split(dirs, ':');
	if (!parts)
		return (NULL);
	hit = NULL;
	i = -1;
	while (parts[++i] && !hit)
	{
		hit = path_join(parts[i], name);
		if (hit && access(hit, F_OK) == 0)
			break ;
		xfree(hit);
		hit = NULL;
	}
	free_tab(parts);
	return (hit);
}

/* autoload [-Uz...] name ... -- source the definition eagerly.
**
** zsh defers: it records the name and reads the file from $fpath on first
** call.  Doing it now instead is a divergence in TIMING only, and it buys
** the property that matters -- `autoload -Uz foo; foo` works -- without a
** second definition-lookup path that could disagree with the first.
**
** A name with nothing on $fpath is NOT an error, matching zsh, which fails
** at the call rather than here.  That is what keeps `autoload -Uz is-at-least`
** from stopping a plugin whose only use of it is a version check it is
** about to guard anyway.
*/
int	builtin_autoload(t_shell *state, t_vec argv)
{
	t_vec	sub;
	char	*path;
	size_t	i;
	char	*two[2];

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-')
		i++;
	while (i < argv.len)
	{
		path = fpath_find(state, ((char **)argv.ctx)[i++]);
		if (!path)
			continue ;
		two[0] = "source";
		two[1] = path;
		sub = (t_vec){.ctx = two, .len = 2, .cap = 2,
			.elem_size = sizeof(char *)};
		builtin_source(state, sub);
		xfree(path);
	}
	return (0);
}

/* The unsupported roster: zmodload, zstyle, compdef, zle, bindkey.
**
** Each configures machinery hellish does not have -- loadable modules, the
** zsh completion system, and the ZLE line editor, which readline has no
** widget or redraw model for.
**
** They are BUILTINS rather than missing commands on purpose.  Left
** unregistered, every one produced `command not found` on a line the plugin
** author wrote deliberately, and the plugin's exit status became 127 -- so a
** plugin that loaded perfectly looked broken.  Registered, the load succeeds
** and the ONE thing that is missing says so by name.
**
** Announced once per name per session, and then quiet: a single completion
** config calls zstyle dozens of times, and forty identical lines is the same
** as no message.  The status is 1 so a caller that checks still can. */
static int	zunsup_slot(const char *name, const char **why)
{
	*why = "the zsh line editor (ZLE)";
	if (!ft_strcmp(name, "zle") || !ft_strcmp(name, "bindkey"))
		return (0);
	*why = "the zsh completion system";
	if (!ft_strcmp(name, "zstyle") || !ft_strcmp(name, "compdef"))
		return (1);
	*why = "loadable modules";
	return (2);
}

int	builtin_zunsupported(t_shell *state, t_vec argv)
{
	static bool	said[3];
	const char	*why;
	int			slot;

	slot = zunsup_slot(((char **)argv.ctx)[0], &why);
	if (!said[slot] && !state->zunsup_quiet)
	{
		said[slot] = true;
		ft_eprintf("%s: %s: not supported (needs %s); "
			"further calls are silent\n", state->ctx,
			((char **)argv.ctx)[0], why);
	}
	return (1);
}
