/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 23:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 23:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "case_match.h"
#include <ctype.h>
#include "libft.h"

/* POSIX character classes inside a bracket expression -- [[:space:]] and
** friends -- for the CASE/trim matcher. Issue #88: this matcher had none,
** so bash-preexec's own
**
**     text="${text#"${text%%[![:space:]]*}"}"
**
** silently trimmed every PROMPT_COMMAND to the empty string, and the
** install string was then appended to the untouched variable with no
** separator: `_hx_precmd_run__bp_install "$_"` at every prompt of a fresh
** install. The FILENAME matcher had classes all along (the golden suite
** pins those at tests/globbing:24), which is exactly the two-matchers
** drift case_match.h warns about.
*/

/* The second half of the class table: display/control classes plus bash's
   own `ascii` and `word`. An unknown name matches nothing, like bash. */
static bool	cm_class_has2(const char *name, int len, char c)
{
	unsigned char	u;

	u = (unsigned char)c;
	if (len == 5 && !ft_strncmp(name, "punct", 5))
		return (ispunct(u) != 0);
	if (len == 5 && !ft_strncmp(name, "print", 5))
		return (isprint(u) != 0);
	if (len == 5 && !ft_strncmp(name, "graph", 5))
		return (isgraph(u) != 0);
	if (len == 5 && !ft_strncmp(name, "cntrl", 5))
		return (iscntrl(u) != 0);
	if (len == 5 && !ft_strncmp(name, "blank", 5))
		return (c == ' ' || c == '\t');
	if (len == 5 && !ft_strncmp(name, "ascii", 5))
		return (u < 128);
	if (len == 6 && !ft_strncmp(name, "xdigit", 6))
		return (isxdigit(u) != 0);
	if (len == 4 && !ft_strncmp(name, "word", 4))
		return (isalnum(u) || c == '_');
	return (false);
}

/* Does character c belong to the class named by name[0..len)? The list is
   bash's: the twelve POSIX classes plus its own `ascii` and `word`. */
static bool	cm_class_has(const char *name, int len, char c)
{
	unsigned char	u;

	u = (unsigned char)c;
	if (len == 5 && !ft_strncmp(name, "alpha", 5))
		return (isalpha(u) != 0);
	if (len == 5 && !ft_strncmp(name, "digit", 5))
		return (isdigit(u) != 0);
	if (len == 5 && !ft_strncmp(name, "alnum", 5))
		return (isalnum(u) != 0);
	if (len == 5 && !ft_strncmp(name, "space", 5))
		return (isspace(u) != 0);
	if (len == 5 && !ft_strncmp(name, "upper", 5))
		return (isupper(u) != 0);
	if (len == 5 && !ft_strncmp(name, "lower", 5))
		return (islower(u) != 0);
	return (cm_class_has2(name, len, c));
}

/* *q points at the '[' of a candidate "[:name:]". Advance past the whole
   class and return true; an unterminated one is bash's ordinary '[' --
   advance one character and return false so the caller treats it as a
   literal member. */
bool	cm_class_skip(const char **q)
{
	const char	*p;

	p = *q + 2;
	while (*p && !(p[0] == ':' && p[1] == ']'))
		p++;
	if (!*p)
		return ((*q)++, false);
	*q = p + 2;
	return (true);
}

/* Match the character c (n bytes) against the "[:name:]" at *pp and
   advance past it. A multibyte character is classified by the wide tables
   (cm_class_has_w). The unterminated case mirrors cm_class_skip: the '['
   is an ordinary member, so it matches a literal '[' and only that. */
bool	cm_class_match(const char *c, size_t n, const char **pp)
{
	const char	*name;
	const char	*p;

	name = *pp + 2;
	p = name;
	while (*p && !(p[0] == ':' && p[1] == ']'))
		p++;
	if (!*p)
		return ((*pp)++, n == 1 && *c == '[');
	*pp = p + 2;
	if (n > 1)
		return (cm_class_has_w(name, (int)(p - name), c, n));
	return (cm_class_has(name, (int)(p - name), *c));
}
