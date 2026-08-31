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

/* Print all readonly variables in `readonly name="value"` form, matching the
   bash output that can be saved and replayed. Variables without a current
   value (declared but not set) are printed without the `="value"` part.
     Quoted, and escaped, for the same reason declare -p is: the output is
   meant to be replayable, and `readonly r=a b` reads back as two names. */
static int	list_readonly(t_shell *state)
{
	size_t	i;
	char	*val;
	char	*q;

	i = 0;
	while (state->readonly_vars.ctx && i < state->readonly_vars.len)
	{
		val = env_expand(state, ((char **)state->readonly_vars.ctx)[i]);
		if (val)
		{
			q = dquote_str(val);
			ft_printf("readonly %s=\"%s\"\n",
				((char **)state->readonly_vars.ctx)[i], q);
			xfree(q);
		}
		else
			ft_printf("readonly %s\n", ((char **)state->readonly_vars.ctx)[i]);
		i++;
	}
	return (0);
}

/* One operand of `readonly`.  Returns what it contributes to the exit
   status: 0, or 1 for a malformed name or an attempt to give a new value
   to a variable that is already read-only.  bash keeps going through the
   remaining operands after either -- `readonly A=1 1BAD B=2` still sets A
   and B and returns 1 -- so this reports rather than bailing out.
   A rejected operand is NOT marked read-only. */
static int	readonly_one(t_shell *state, char *arg)
{
	char	*eq;
	char	*name;
	int		rc;

	eq = ft_strchr(arg, '=');
	if (eq)
		name = ft_strndup(arg, eq - arg);
	else
		name = ft_strdup(arg);
	rc = 0;
	if (!ft_is_valid_ident(name))
		rc = (ft_eprintf("%s: readonly: `%s': not a valid identifier\n",
					state->ctx, arg), 1);
	else if (eq && is_readonly_var(state, name))
		rc = (ft_eprintf("%s: readonly: %s: readonly variable\n",
					state->ctx, name), 1);
	else if (eq)
		env_set(&state->env, env_create(ft_strdup(name),
				ft_strdup(eq + 1), false));
	if (rc == 0)
		mark_readonly(state, name);
	return (xfree(name), rc);
}

/* readonly [-p] [--] [name[=value] ...]: mark variables read-only.
   An unknown option is a usage error (status 2) and stops the command,
   which is how bash reports it; every other failure is per-operand. */
int	builtin_readonly(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	int		rc;

	av = (char **)argv.ctx;
	if (argv.len == 1 || (argv.len == 2 && !ft_strcmp(av[1], "-p")))
		return (list_readonly(state));
	rc = 0;
	i = 0;
	while (++i < argv.len)
	{
		if (!ft_strcmp(av[i], "-p") || !ft_strcmp(av[i], "--"))
			continue ;
		if (av[i][0] == '-' && av[i][1])
			return (ft_eprintf("%s: readonly: %s: invalid option\n",
					state->ctx, av[i]), 2);
		rc |= readonly_one(state, av[i]);
	}
	return (rc);
}
