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

/* alias [name[=value] ...]: define or display aliases. `alias` alone lists
   all current aliases. Per argument: `alias ll` prints it, `alias ll='ls
   -la'` defines it. Returns 1 if any name was not found (print-only form),
   0 otherwise. The status accumulates — a mixed call like `alias x ll`
   returns 1 only if `ll` was not defined. */
int	builtin_alias(t_shell *state, t_vec argv)
{
	size_t	i;
	int		ret;

	if (argv.len <= 1)
	{
		alias_print_all(&state->aliases);
		return (0);
	}
	ret = 0;
	i = 1;
	while (i < argv.len)
	{
		if (set_alias_arg(state, ((char **)argv.ctx)[i]))
			ret = 1;
		i++;
	}
	return (ret);
}
