/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   omz_shim2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"
#include <unistd.h>

/* The helpers of omz_shim.c; the design note is there. */

/* The one file the shim recognises: oh-my-zsh.sh by name, wherever $ZSH
   points.  A path is matched, never content, for the reason zsh_mode.c
   gives: the same bytes must always mean the same thing. */
bool	omz_loader_path(const char *path)
{
	const char	*base;

	if (!path)
		return (false);
	base = ft_strrchr((char *)path, '/');
	if (base)
		base++;
	else
		base = path;
	return (!ft_strcmp(base, "oh-my-zsh.sh"));
}

/* Source one file through the ordinary builtin so it gets a frame, $0,
   and the .zsh extension rule exactly like a plugin the user sourced. */
void	omz_source(t_shell *state, char *path)
{
	char	*two[2];
	t_vec	sub;

	two[0] = "source";
	two[1] = path;
	sub = (t_vec){.ctx = two, .len = 2, .cap = 2,
		.elem_size = sizeof(char *)};
	builtin_source(state, sub);
}

/* Plugins that are the zsh line editor itself.  Every one of them binds
   widgets, hooks the redraw or rewrites the buffer; without ZLE there is
   nothing to load and a screen of "not supported" to print.  Skipped by
   name, silently -- the corpus records the same verdict as
   "unsupported" for the ones it runs. */
bool	omz_zle_only(const char *name)
{
	static const char	*t[] = {"zsh-autosuggestions",
		"zsh-syntax-highlighting", "fast-syntax-highlighting",
		"zsh-history-substring-search", "history-substring-search",
		"zsh-vi-mode", "vi-mode", "fzf-tab", "zsh-autocomplete",
		"zsh-navigation-tools", "zsh-completions", NULL};
	int					i;

	i = -1;
	while (t[++i])
		if (!ft_strcmp(t[i], name))
			return (true);
	return (false);
}

/* $ZSH_CUSTOM/plugins/NAME/NAME.plugin.zsh, else $ZSH/plugins/..., else
   NULL.  Owned. */
char	*omz_plugin_file(t_omz *o, const char *name)
{
	char	*p;

	p = ft_asprintf("%s/plugins/%s/%s.plugin.zsh", o->custom, name, name);
	if (p && access(p, R_OK) == 0)
		return (p);
	xfree(p);
	p = ft_asprintf("%s/plugins/%s/%s.plugin.zsh", o->zsh, name, name);
	if (p && access(p, R_OK) == 0)
		return (p);
	xfree(p);
	return (NULL);
}

/* $ZSH_CUSTOM/ *.zsh in lexical order: the user's own additions, which
   oh-my-zsh.sh sources after the plugins. */
void	omz_custom(t_shell *state, t_omz *o)
{
	t_vec	files;
	size_t	i;

	vec_init(&files);
	files.elem_size = sizeof(char *);
	collect(o->custom, ".zsh", &files);
	sort_strvec(&files, 0);
	i = 0;
	while (i < files.len)
	{
		omz_source(state, ((char **)files.ctx)[i]);
		xfree(((char **)files.ctx)[i++]);
	}
	xfree(files.ctx);
}
