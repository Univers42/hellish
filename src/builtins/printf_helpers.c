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

/* \NNN in a FORMAT string is one to three octal digits, the leading zero
   counting as one of them -- C's rule, and bash's tescape() outside %b:
   "\0337" is ESC then '7'.  In a %b ARGUMENT the leading zero is a marker
   with up to three digits after it (echo -e's \0nnn), so the same "\0337"
   is the single byte 0337.  Reading the zero as a marker in both places
   put a raw 0xDF on the terminal where born2root's build dashboard meant
   to save the cursor (printf "\0337" before every spinner frame), and
   every restore that followed jumped to a position nobody had saved. */
static char	pf_read_octal(const char *s, int *i, bool in_b)
{
	int	v;
	int	n;

	v = 0;
	n = 0;
	if (in_b && s[*i] == '0')
		(*i)++;
	while (n < 3 && s[*i] >= '0' && s[*i] <= '7')
	{
		v = v * 8 + (s[(*i)++] - '0');
		n++;
	}
	return ((char)v);
}

/* \xHH: one or two hex digits.  With none at all bash keeps the \x as
   text and says so on stderr -- `printf '\xg'` prints \xg, warns
   "missing hex digit for \x", and still exits 0.  We used to hand back a
   NUL for it, which cut a %b argument short right there.  *i is left on
   the 'x' so the caller's loop emits it as the plain character it is. */
static char	pf_read_hex(t_pf *pf, const char *s, int *i)
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
	if (n == 0)
	{
		ft_eprintf("%s: printf: missing hex digit for \\x\n", pf->ctx);
		return ((*i)--, '\\');
	}
	return ((char)v);
}

/* Resolve a backslash escape at s[*i] ('\' already seen); advance *i past
   it.  in_b says whether s is a %b ARGUMENT rather than the format string:
   only there does \c stop all further output (recorded in pf->stop), and
   only there does a leading zero open the four-character \0nnn octal form.
   bash draws both lines in the same place (printf.def's tescape, sawc). */
char	pf_escape(t_pf *pf, const char *s, int *i, bool in_b)
{
	static const char	from[] = "ntr\\abfveE";
	static const char	to[] = "\n\t\r\\\a\b\f\v\033\033";
	char				*p;

	(*i)++;
	p = ft_strchr(from, s[*i]);
	if (s[*i] && p)
		return ((*i)++, to[p - from]);
	if (s[*i] == 'c' && in_b)
		return (pf->stop = true, '\0');
	if (s[*i] == 'x')
		return (pf_read_hex(pf, s, i));
	if (s[*i] >= '0' && s[*i] <= '7')
		return (pf_read_octal(s, i, in_b));
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

/* %b : emit arg interpreting backslash escapes (\c aborts the whole
   printf).  Bytes go out one at a time and a NUL among them is a byte like
   any other -- `printf '%b' 'a\0b'` is three bytes in bash, and it used to
   be one here because everything downstream measured with strlen. */
void	pf_emit_b(t_pf *pf, t_string *out, const char *arg)
{
	int		i;
	char	c;

	i = 0;
	while (arg && arg[i])
	{
		if (arg[i] == '\\' && arg[i + 1])
		{
			c = pf_escape(pf, arg, &i, true);
			if (pf->stop)
				return ;
			vec_push_char(out, c);
		}
		else
			vec_push_char(out, arg[i++]);
	}
}
