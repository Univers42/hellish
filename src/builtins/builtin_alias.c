/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_alias.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_alias.h"

/* Process one alias argument. If it contains '=', define the alias
   (name=value). Without '=', print the current definition or an error if
   the name is not found — exit status 1 in that case, matching bash. */
static int	set_alias_arg(t_shell *state, const char *arg)
{
	char	*eq;
	char	*name;
	char	*value;

	eq = ft_strchr(arg, '=');
	if (!eq)
	{
		if (alias_print_one(&state->aliases, arg))
		{
			ft_eprintf("%s: alias: %s: not found\n", state->ctx, arg);
			return (1);
		}
		return (0);
	}
	name = ft_substr(arg, 0, eq - arg);
	value = ft_strdup(eq + 1);
	alias_set(&state->aliases, name, value);
	xfree(name);
	xfree(value);
	return (0);
}

/* zsh's option letters, honoured in the dialect only.  -g (global alias)
   is an ordinary alias here: its one difference, expanding anywhere in
   the line, has no home in this shell, and the command-position use is
   what plugins rely on (oh-my-zsh's directories.zsh: `alias -g ...='cd
   ../..'`).  -s (suffix alias: `alias -s txt=cat` runs cat on any
   `x.txt` typed as a command) needs zsh's command lookup and is accepted
   silently, the way zstyle is.  -L, -m, -r are list forms and print as
   `alias` prints.  Each of these used to fail as "alias: -g: not found",
   once per line, at every start of a shell that loads such a plugin. */
static size_t	alias_zsh_opts(t_shell *state, t_vec argv, bool *skip)
{
	size_t	i;
	char	*w;

	*skip = false;
	i = 1;
	if (!zsh_mode(state))
		return (i);
	while (i < argv.len)
	{
		w = ((char **)argv.ctx)[i];
		if (w[0] != '-' || !w[1] || !ft_strcmp(w, "--"))
			break ;
		if (ft_strchr(w, 's'))
			*skip = true;
		i++;
	}
	return (i);
}

/* alias [name[=value] ...]: define or display aliases. `alias` alone lists
   all current aliases. Per argument: `alias ll` prints it, `alias ll='ls
   -la'` defines it. Returns 1 if any name was not found (print-only form),
   0 otherwise. The status accumulates — a mixed call like `alias x ll`
   returns 1 only if `ll` was not defined. */
int	builtin_alias(t_shell *state, t_vec argv)
{
	size_t	i;
	int		ret;
	bool	skip;

	if (argv.len <= 1 || !ft_strcmp(((char **)argv.ctx)[1], "-p"))
	{
		alias_print_all(&state->aliases, argv.len > 1);
		return (0);
	}
	ret = 0;
	i = alias_zsh_opts(state, argv, &skip);
	if (skip)
		return (0);
	if (i < argv.len && !ft_strcmp(((char **)argv.ctx)[i], "--"))
		i++;
	while (i < argv.len)
	{
		if (set_alias_arg(state, ((char **)argv.ctx)[i]))
			ret = 1;
		i++;
	}
	return (ret);
}
