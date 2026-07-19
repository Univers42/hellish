/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_helpers.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"

static char	pf_read_octal(const char *s, int *i)
{
	int	v;
	int	n;

	v = 0;
	n = 0;
	if (s[*i] == '0')
		(*i)++;
	while (n < 3 && s[*i] >= '0' && s[*i] <= '7')
	{
		v = v * 8 + (s[(*i)++] - '0');
		n++;
	}
	return ((char)v);
}

static char	pf_read_hex(const char *s, int *i)
{
	int	v;
	int	n;
	int	c;

	v = 0;
	n = 0;
	(*i)++;
	c = ft_tolower(s[*i]);
	while (n < 2 && (ft_isdigit(c) || (c >= 'a' && c <= 'f')))
	{
		if (ft_isdigit(c))
			v = v * 16 + (c - '0');
		else
			v = v * 16 + (c - 'a' + 10);
		c = ft_tolower(s[++(*i)]);
		n++;
	}
	return ((char)v);
}

/* Resolve a backslash escape at s[*i] ('\' already seen); advance *i past it.
   Sets *stop on \c (abort all further output) — but only when the caller
   provides a stop channel: bash honours \c inside %b arguments only, so the
   format-string pass hands us NULL and \c falls through to the literal
   backslash path (the 'c' is then emitted as a plain character). */
char	pf_escape(const char *s, int *i, bool *stop)
{
	(*i)++;
	if (s[*i] == 'n')
		return ((*i)++, '\n');
	if (s[*i] == 't')
		return ((*i)++, '\t');
	if (s[*i] == 'r')
		return ((*i)++, '\r');
	if (s[*i] == '\\')
		return ((*i)++, '\\');
	if (s[*i] == 'a')
		return ((*i)++, '\a');
	if (s[*i] == 'b')
		return ((*i)++, '\b');
	if (s[*i] == 'f')
		return ((*i)++, '\f');
	if (s[*i] == 'v')
		return ((*i)++, '\v');
	if (s[*i] == 'c' && stop)
		return (*stop = true, '\0');
	if (s[*i] == 'x')
		return (pf_read_hex(s, i));
	if (s[*i] >= '0' && s[*i] <= '7')
		return (pf_read_octal(s, i));
	return ('\\');
}

/* Convert a printf integer argument with bash's strict rules: leading
   blanks, an optional sign and a C-style base prefix (0x.., 0..) are fine,
   but anything trailing — including spaces and 64#a base literals — is an
   error, as are empty strings and out-of-range values. bash still uses the
   converted prefix (clamped on overflow) and only exits 1 at the end, so we
   report through pf_err_num and return the value regardless. A leading
   single or double quote yields the next character's code point; a missing
   (NULL) argument is silently zero. */
long long	pf_num(t_pf *pf, const char *arg)
{
	char		*end;
	long long	v;

	if (!arg)
		return (0);
	if (arg[0] == '\'' || arg[0] == '"')
		return ((unsigned char)arg[1]);
	errno = 0;
	v = strtoll(arg, &end, 0);
	if (end == arg || *end != '\0' || errno == ERANGE)
		pf_err_num(pf, arg);
	return (v);
}

/* %b : emit arg interpreting backslash escapes (\c aborts the whole printf). */
void	pf_emit_b(t_string *out, const char *arg, bool *stop)
{
	int		i;
	char	c;

	i = 0;
	while (arg && arg[i])
	{
		if (arg[i] == '\\' && arg[i + 1])
		{
			c = pf_escape(arg, &i, stop);
			if (*stop)
				return ;
			vec_push_char(out, c);
		}
		else
			vec_push_char(out, arg[i++]);
	}
}
