/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_quote.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"

/* `printf %q` -- render an argument so that re-reading it as shell input
** yields the identical string.  It failed loudly ("`%q': invalid format
** character", status 1), which is honest but leaves generated scripts with
** no safe way to embed a filename: %q IS the quoting helper, so its absence
** has no workaround inside the shell.
**
** Two output shapes, and bash picks by content, not by preference:
**   - any control byte present  ->  the ANSI-C form  $'a\tb'
**   - otherwise                 ->  backslash escapes  a\ b
** and the empty string is ''  (nothing else survives a round trip).
**
** Every character in the escape sets below was measured against
** bash 5.3.9 one byte at a time, at index 0 and mid-string, rather than
** copied from the manual: `#` and `~` are shell-special only in the first
** position, `,` is special in both, and `%` `+` `-` `.` `/` `:` `=` `@`
** are special in neither -- a set that is easy to get subtly wrong and
** whose errors are invisible until a round trip breaks.
*/

/* The ANSI-C form is required as soon as one byte cannot be written
   literally between quotes. DEL is in the set for the same reason the
   C0 range is: it is unprintable, so an editor would silently eat it. */
static bool	pq_needs_ansi(const char *s)
{
	int	i;

	i = 0;
	while (s[i])
	{
		if ((unsigned char)s[i] < 32 || (unsigned char)s[i] == 127)
			return (true);
		i++;
	}
	return (false);
}

/* The backslash form: escape what the shell would otherwise interpret.
   `#` and `~` earn a backslash only at index 0, where a comment and a
   tilde expansion would start. */
static void	pq_plain(t_string *out, const char *s)
{
	int	i;

	i = 0;
	while (s[i])
	{
		if (ft_strchr(" !\"$&'()*,;<>?[\\]^`{|}", s[i])
			|| (i == 0 && (s[i] == '#' || s[i] == '~')))
			vec_push_char(out, '\\');
		vec_push_char(out, s[i]);
		i++;
	}
}

/* Quote `arg` for re-reading; the caller owns the result. */
char	*pf_quote(const char *arg)
{
	t_string	out;

	vec_init(&out);
	out.elem_size = 1;
	if (!arg || !*arg)
		vec_push_str(&out, "''");
	else if (pq_needs_ansi(arg))
		pq_ansi(&out, arg);
	else
		pq_plain(&out, arg);
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

/* %q: quote the argument, then emit it through the ordinary %s machinery
   so a field width still applies (`printf '%-12q'` pads the quoted form,
   which is what makes %q usable for building aligned scripts). */
void	pf_conv_quote(t_pf *pf, t_spec *sp, const char *arg)
{
	char	fmt[80];
	char	*q;

	q = pf_quote(arg);
	pf_build_spec(fmt, sp, 's');
	pf_emit_sized(pf, sp, fmt, q);
	xfree(q);
}
