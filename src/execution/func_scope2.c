/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   func_scope2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

void	try_unset(t_shell *state, char *key);

void	restore_temp_assigns(t_shell *state, t_vec *saves)
{
	size_t	i;

	i = saves->len;
	while (i > 0)
		restore_one(state, (t_scope_save *)vec_idx(saves, --i));
	free(saves->ctx);
}

static void	local_set_var(t_shell *state, char *key, char *eq)
{
	if (eq)
		env_set(&state->env,
			env_create(key, ft_strdup(eq + 1), false));
	else
		env_set(&state->env, env_create(key, ft_strdup(""), false));
}

/* local name[=value] ... : make each name local to the current function. */
int	builtin_local(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	char	*eq;
	char	*key;

	av = (char **)argv.ctx;
	if (state->func_depth <= 0)
	{
		ft_eprintf("%s: local: can only be used in a function\n", state->ctx);
		return (1);
	}
	i = 1;
	while (i < argv.len)
	{
		eq = ft_strchr(av[i], '=');
		if (eq)
			key = ft_strndup(av[i], eq - av[i]);
		else
			key = ft_strdup(av[i]);
		scope_save(state, key);
		local_set_var(state, key, eq);
		i++;
	}
	return (0);
}
