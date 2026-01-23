/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   export_helpers.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:02:29 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:59:24 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

void	parse_export_arg(char *str, char **ident, char **val)
{
	char	*eq;

	eq = ft_strchr(str, '=');
	if (eq)
	{
		*ident = ft_strndup(str, eq - str);
		*val = ft_strdup(eq + 1);
	}
	else
	{
		*ident = ft_strdup(str);
		*val = NULL;
	}
}

bool	ft_is_valid_ident(char *id)
{
	int	i;

	i = 0;
	if (!is_var_name_p1(id[0]))
		return (false);
	while (id[i] && is_var_name_p2(id[i]))
		i++;
	return (!id[i]);
}

char	strip_surrounding_quotes(char **val)
{
	size_t	vlen;
	char	*clean;
	char	f;

	if (!val || !*val)
		return (0);
	vlen = ft_strlen(*val);
	if (vlen >= 2)
	{
		f = (*val)[0];
		if ((f == '"' || f == '\'') && (*val)[vlen - 1] == f)
		{
			clean = ft_strndup(*val + 1, vlen - 2);
			free(*val);
			*val = clean;
			return (f);
		}
	}
	return (0);
}

void	consume_following_value(t_vec av, int *i, char **val)
{
	int		idx;
	char	*next;

	idx = *i;
	if ((!*val || (*val)[0] == '\0') && idx + 1 < (int)av.len)
	{
		next = ((char **)av.ctx)[idx + 1];
		if (next && !ft_strchr(next, '='))
		{
			*val = ft_strdup(next);
			*i = idx + 1;
		}
	}
}
