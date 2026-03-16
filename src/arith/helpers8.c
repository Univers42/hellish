/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers8.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:15:22 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:46:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

void	set_var_value(t_arith_parser *p, const char *name, int len,
	long long val)
{
	char	*key;
	char	buf[32];
	int		i;
	int		neg;

	if (p->no_side_effects)
		return ;
	key = ft_strndup(name, len);
	if (!key)
		return ;
	neg = 0;
	if (val < 0)
	{
		neg = 1;
		val = -val;
	}
	i = 31;
	buf[i] = '\0';
	if (val == 0)
		buf[--i] = '0';
	while (val > 0)
	{
		buf[--i] = '0' + (val % 10);
		val /= 10;
	}
	if (neg)
		buf[--i] = '-';
	env_set(&p->shell->env,
		env_create(key, ft_strdup(buf + i), true));
}

long long	get_var_value(t_arith_parser *p, const char *name, int len)
{
	char	*val;
	char	*key;
	int		result;

	key = ft_strndup(name, len);
	if (!key)
		return (0);
	val = env_expand_n(p->shell, key, len);
	free(key);
	if (!val || !*val)
		return (0);
	if (ft_checked_atoi(val, &result, 42) != 0)
		return (0);
	return ((long long)result);
}

void	expect(t_arith_parser *p, t_arith_tok type)
{
	if (p->lexer->current.type != type)
	{
		p->error = true;
		return ;
	}
	arith_lexer_advance(p->lexer);
}
