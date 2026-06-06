/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_readonly.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Check whether `key` is in the readonly list. Linear scan is fine because
   the list is short (usually < 20 entries even in a complex script). This is
   called from env_set to block assignments; if it returns true, the caller
   should print an error and skip the write. */
bool	is_readonly_var(t_shell *state, const char *key)
{
	size_t	i;

	if (!state->readonly_vars.ctx)
		return (false);
	i = 0;
	while (i < state->readonly_vars.len)
	{
		if (!ft_strcmp(((char **)state->readonly_vars.ctx)[i], key))
			return (true);
		i++;
	}
	return (false);
}

/* Add `name` to the readonly list. We strdup it so the original string (from
   the argument vector) can be freed. The vec is lazy-initialised on first use
   so the common case (no readonly variables) pays nothing. */
static void	mark_readonly(t_shell *state, char *name)
{
	char	*dup;

	if (is_readonly_var(state, name))
		return ;
	if (!state->readonly_vars.ctx)
	{
		vec_init(&state->readonly_vars);
		state->readonly_vars.elem_size = sizeof(char *);
	}
	dup = ft_strdup(name);
	vec_push(&state->readonly_vars, &dup);
}

/* Print all readonly variables in `readonly name=value` form, matching the
   bash output that can be saved and replayed. Variables without a current
   value (declared but not set) are printed without the `=value` part. */
static int	list_readonly(t_shell *state)
{
	size_t	i;
	char	*val;

	i = 0;
	while (state->readonly_vars.ctx && i < state->readonly_vars.len)
	{
		val = env_expand(state, ((char **)state->readonly_vars.ctx)[i]);
		if (val)
			ft_printf("readonly %s=%s\n",
				((char **)state->readonly_vars.ctx)[i], val);
		else
			ft_printf("readonly %s\n", ((char **)state->readonly_vars.ctx)[i]);
		i++;
	}
	return (0);
}

/* readonly [-p] [name[=value] ...]: mark variables read-only. */
int	builtin_readonly(t_shell *state, t_vec argv)
{
	char	**av;
	char	*eq;
	char	*name;
	size_t	i;

	av = (char **)argv.ctx;
	if (argv.len == 1 || (argv.len == 2 && !ft_strcmp(av[1], "-p")))
		return (list_readonly(state));
	i = 0;
	while (++i < argv.len)
	{
		eq = ft_strchr(av[i], '=');
		if (eq)
			name = ft_strndup(av[i], eq - av[i]);
		else
			name = ft_strdup(av[i]);
		if (eq && !is_readonly_var(state, name))
			env_set(&state->env, env_create(ft_strdup(name),
					ft_strdup(eq + 1), false));
		mark_readonly(state, name);
		xfree(name);
	}
	return (0);
}
