/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers8.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:15:22 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:15:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

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
