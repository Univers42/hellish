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

/* Decimal text of val into buf[32], returning the start (handles LLONG_MIN by
   formatting the magnitude as unsigned). */
static char	*ll_to_buf(char *buf, long long val)
{
	int					i;
	int					neg;
	unsigned long long	u;

	neg = (val < 0);
	u = (unsigned long long)val;
	if (neg)
		u = -u;
	i = 31;
	buf[i] = '\0';
	if (u == 0)
		buf[--i] = '0';
	while (u > 0)
	{
		buf[--i] = '0' + (int)(u % 10);
		u /= 10;
	}
	if (neg)
		buf[--i] = '-';
	return (buf + i);
}

void	set_var_value(t_arith_parser *p, const char *name, int len,
	long long val)
{
	char	buf[32];
	char	*key;

	if (p->no_side_effects)
		return ;
	key = ft_strndup(name, len);
	if (!key)
		return ;
	env_set(&p->shell->env, env_create(key, ft_strdup(ll_to_buf(buf, val)),
			true));
}

/* A plain base-10 integer (no leading zero, so octal "010" is excluded). */
static int	is_simple_decimal(const char *s)
{
	if (*s == '-' || *s == '+')
		s++;
	if (*s == '0')
		return (s[1] == '\0');
	if (!ft_isdigit((unsigned char)*s))
		return (0);
	while (*s)
	{
		if (!ft_isdigit((unsigned char)*s++))
			return (0);
	}
	return (1);
}

/* A variable's value may itself be an arithmetic expression / another name
   (POSIX: recursive evaluation), e.g. x=y; y=5 -> $((x))==5. Bounded depth
   guards against cycles like x=x. */
static long long	resolve_recursive(t_arith_parser *p, const char *val)
{
	static int	depth;
	char		*dup;
	long long	r;
	bool		err;

	if (depth >= 100)
		return (p->error = true, 0);
	dup = ft_strdup(val);
	if (!dup)
		return (0);
	depth++;
	err = false;
	r = arith_eval(p->shell, dup, (int)ft_strlen(dup), &err);
	depth--;
	free(dup);
	if (err)
		p->error = true;
	return (r);
}

long long	get_var_value(t_arith_parser *p, const char *name, int len)
{
	char	*val;
	char	*key;

	key = ft_strndup(name, len);
	if (!key)
		return (0);
	val = env_expand_n(p->shell, key, len);
	free(key);
	if (!val || !*val)
		return (0);
	if (is_simple_decimal(val))
		return (ft_atol(val));
	return (resolve_recursive(p, val));
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
