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
