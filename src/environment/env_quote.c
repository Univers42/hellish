/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_quote.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "libft.h"

/* The quoting `declare -p` and `set` owe their output.  Both print inside
   double quotes, and until this file existed neither escaped anything, so
     s='x"y'; declare -p s
   printed  declare -- s="x"y"  -- which does not read back as what it
   describes.  That is the point of declare -p: bash's output is valid input,
   and ours was not, silently, for every value containing " $ ` or \.

   One routine, three call sites (scalar, indexed element, assoc key+value),
   because the alternative is three subtly different escapes. */

/* Inside bash's double quotes only four characters keep a special meaning,
   so only those four are escaped -- ' and ! and whitespace pass through
   verbatim, which is why `declare -p` shows x'y unquoted. */
void	vec_push_dquoted(t_string *out, const char *s, int len)
{
	int	i;

	i = 0;
	while (s && i < len)
	{
		if (s[i] == '"' || s[i] == '$' || s[i] == '`' || s[i] == '\\')
			vec_push_char(out, '\\');
		vec_push_char(out, s[i++]);
	}
}

/* The same escape for the callers that print with ft_printf rather than
   build a t_string: declare -p, export -p, readonly -p.  Returns an owned
   string, "" for a NULL value so the callers keep their existing shape. */
char	*dquote_str(const char *s)
{
	t_string	out;

	vec_init(&out);
	out.elem_size = 1;
	if (s)
		vec_push_dquoted(&out, s, (int)ft_strlen(s));
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

/* Does this associative KEY have to be quoted at all?  bash prints [abc],
   [a-b], [a/b.c] and [5] bare and only quotes when the key holds something
   the reader would otherwise take as syntax.  Measured against bash 5.3.9
   one character at a time rather than copied from memory: `,` `=` `:` `+`
   `%` `_` and `-` are NOT in the set, though several of them look like they
   should be. */
static bool	key_meta(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\'' || c == '"'
		|| c == '\\' || c == '|' || c == '&' || c == ';' || c == '('
		|| c == ')' || c == '<' || c == '>' || c == '!' || c == '{'
		|| c == '}' || c == '*' || c == '[' || c == '?' || c == ']'
		|| c == '^' || c == '$' || c == '`');
}

/* Three characters are special only by POSITION, not everywhere:
     @   only as the WHOLE key, where [@] would read as "every element"
         -- a@b prints bare, @ does not;
     ~ # only at the start, where they would be tilde expansion or a
         comment -- a~ and a# print bare, ~ and # do not.
   Each of the three was measured; `*` looks like it belongs here too and
   does not (a*b IS quoted), which is why they are listed rather than
   generalised. */
bool	assoc_key_quoted(const char *k, int len)
{
	int	i;

	if (len == 1 && k[0] == '@')
		return (true);
	i = 0;
	while (i < len)
	{
		if (key_meta(k[i]))
			return (true);
		if (i == 0 && (k[i] == '~' || k[i] == '#'))
			return (true);
		i++;
	}
	return (false);
}
