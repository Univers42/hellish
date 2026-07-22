/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_declare2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"
#include "arith.h"

/* declare -n ref[=target]: mark `ref` a nameref aliasing `target`. Reads
   and writes of ref redirect to target (env_expand / apply_var_attrs).
   `declare -n ref=target` sets the target now; `declare -n ref` (bare)
   just records the attribute for a later assignment to fill in. */
int	declare_nameref(t_shell *state, t_vec argv, size_t i)
{
	char	*eq;
	char	*name;

	while (i < argv.len)
	{
		if (((char **)argv.ctx)[i][0] == '-')
			return (i++, (i < argv.len) ? declare_nameref(state, argv, i) : 0);
		eq = ft_strchr(((char **)argv.ctx)[i], '=');
		if (eq)
		{
			name = ft_strndup(((char **)argv.ctx)[i],
					eq - ((char **)argv.ctx)[i]);
			attr_set(state, name, 'n', eq + 1);
			xfree(name);
		}
		else
			attr_set(state, ((char **)argv.ctx)[i], 'n', "");
		i++;
	}
	return (0);
}

/* Arithmetic-evaluate `expr` to a fresh decimal string ("0" on error). */
static char	*int_eval(t_shell *state, const char *expr)
{
	char	*res;

	res = arith_expand(state, expr, (int)ft_strlen(expr));
	if (!res)
		return (ft_strdup("0"));
	return (res);
}

/* declare -i name[=expr]: mark `name` integer; a present value is
   arithmetic-evaluated now, and every later assignment to it is too
   (apply_var_attrs). */
int	declare_integer(t_shell *state, t_vec argv, size_t i)
{
	char	*eq;
	char	*name;
	char	*val;

	while (i < argv.len)
	{
		if (((char **)argv.ctx)[i][0] == '-')
		{
			i++;
			continue ;
		}
		eq = ft_strchr(((char **)argv.ctx)[i], '=');
		if (!eq)
			attr_set(state, ((char **)argv.ctx)[i], 'i', NULL);
		else
		{
			name = ft_strndup(((char **)argv.ctx)[i],
					eq - ((char **)argv.ctx)[i]);
			attr_set(state, name, 'i', NULL);
			val = int_eval(state, eq + 1);
			env_set(&state->env, env_create(name, val, false));
		}
		i++;
	}
	return (0);
}
