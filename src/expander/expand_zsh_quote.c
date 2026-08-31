/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_quote.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:55:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:55:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* (q): quote the value so it survives a re-parse.  zsh has FOUR spellings
   and they are not interchangeable, which is easy to miss because each one
   alone looks right:
     (q)    backslash each special       a\ b     it\'s
     (qq)   single quotes                'a b'
     (qqq)  double quotes                "a b"
     (q-)   the shortest that works      'a b'
   `q` counts its own repeats, so the style is the repeat count.  Bare (q)
   is the common one in plugin code and is the one that is NOT the obvious
   quoting -- assuming single quotes there produces output that reparses to
   the same string but does not compare equal to what zsh printed, which is
   how it survives a test that only round-trips. */
static void	zq_backslash(t_string *out, const char *v)
{
	while (*v)
	{
		if (!ft_isalnum((unsigned char)*v) && !ft_strchr("_./-+,:@%^=", *v))
			vec_push_char(out, '\\');
		vec_push_char(out, *v);
		v++;
	}
}

char	*zf_quote(const char *v, char style)
{
	t_string	out;

	vec_init(&out);
	out.elem_size = 1;
	if (style == '\\')
		zq_backslash(&out, v);
	else if (style == '"')
		(vec_push_char(&out, '"'), vec_push_str(&out, (char *)v),
			vec_push_char(&out, '"'));
	else
	{
		vec_push_char(&out, '\'');
		while (*v)
		{
			if (*v == '\'')
				vec_push_str(&out, "'\\''");
			else
				vec_push_char(&out, *v);
			v++;
		}
		vec_push_char(&out, '\'');
	}
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

int	zf_count(const t_zflags *f, char c)
{
	int	i;
	int	n;

	i = 0;
	n = 0;
	while (i < f->n)
	{
		if (f->set[i] == c)
			n++;
		i++;
	}
	return (n);
}

/* Which of zsh's four quoting styles (q) asked for: the repeat count picks
   it, and a trailing `-` is the "shortest that works" form, which for every
   value in the corpus comes out as single quotes. */
char	zq_style(const t_zflags *f)
{
	if (zf_has(f, '-') || zf_count(f, 'q') == 2)
		return ('\'');
	if (zf_count(f, 'q') >= 3)
		return ('"');
	return ('\\');
}
