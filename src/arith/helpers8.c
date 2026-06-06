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

/* Fill buf[32] with the decimal representation of val, right-to-left.
   Returns a pointer to the start of the number inside buf. The unsigned-
   magnitude trick handles LLONG_MIN whose absolute value can't be represented
   as a positive signed long long. This is the inline version used to format
   the new value for env_set; the public arith_lltoa in eval.c is a strdup
   wrapper around the same logic. */
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

/* Write `val` back into the shell environment under the given variable name.
   The no_side_effects guard is the key: dead branches of a ternary, or the
   false side of && / ||, call this with no_side_effects=true and their
   assignments are silently dropped. That's the POSIX-required behaviour and
   what makes `x=1; (( 0 && (x=99) )); echo $x` print 1, not 99. */
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

/* Return true when `s` looks like a plain base-10 integer (no leading zero,
   so "010" is excluded -- that would be an octal literal if re-evaluated).
   An optional leading sign is accepted. When this check passes, ft_atol is
   enough and we avoid the more expensive recursive arith_eval call. */
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

/* A variable's value may be another name or an expression -- POSIX allows
   `x=y; y=5; echo $((x))` to print 5 via recursive evaluation. We dup the
   string because arith_eval may modify the input buffer. The static `depth`
   counter caps recursion at 100 levels to kill cycles like `x=x` before
   we blow the C stack. depth is shared across all active evaluations in the
   process, so nested $((...)) in subshells is still bounded. */
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
	xfree(dup);
	if (err)
		p->error = true;
	return (r);
}

/* Look up a variable in the shell environment and return its numeric value.
   An unset or empty variable is 0 (POSIX). A simple decimal string is parsed
   with ft_atol (fast path). Anything else -- hex, octal, another name, an
   expression -- goes through resolve_recursive which re-invokes the full
   arithmetic evaluator. That's what makes `((16#ff))` and indirect
   references work. */
long long	get_var_value(t_arith_parser *p, const char *name, int len)
{
	char	*val;

	val = env_expand_n(p->shell, (char *)name, len);
	if (!val || !*val)
		return (0);
	if (is_simple_decimal(val))
		return (ft_atol(val));
	return (resolve_recursive(p, val));
}
