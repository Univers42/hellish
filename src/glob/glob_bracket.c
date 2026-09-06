/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_bracket.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 13:14:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/* Expand a character range (e.g. 'a'-'z') into individual bytes in `buf`
   starting at buf_pos. Returns the count of characters added, or -1 when
   start > end (invalid range; the caller reverts to treating the dash as a
   literal). POSIX says ranges are locale-dependent; we use byte order
   (matching the C locale and bash --posix) to avoid locale-induced surprises
   on UTF-8 systems. */
int	expand_range(char start, char end, char *buf, int buf_pos)
{
	char	c;
	int		count;

	count = 0;
	if (start > end)
		return (-1);
	c = start;
	while (c <= end)
	{
		buf[buf_pos + count] = c;
		count++;
		c++;
	}
	return (count);
}

/* Check whether `s` starts with a POSIX character class like [:alpha:], and
   if so expand the class members into `buf` at *buf_pos. Returns the number
   of bytes consumed from `s` (the full "[:name:]" length) so the caller can
   skip past the class, or 0 if the prefix doesn't match any known class.
   The class member strings are pre-built ASCII sets stored in arith_private.h
   via get_classes_singleton() -- no locale calls, no dynamic allocation. */
int	check_posix_class(const char *s, int len, char *buf, int *buf_pos)
{
	int						i;
	size_t					chars_len;
	const struct s_classes	*classes = get_classes_singleton();

	if (len < 2 || s[0] != '[' || s[1] != ':')
		return (0);
	i = 0;
	while (classes[i].pattern)
	{
		if (len >= classes[i].plen
			&& ft_strncmp(s, classes[i].pattern, classes[i].plen) == 0)
		{
			chars_len = ft_strlen(classes[i].chars);
			ft_memcpy(buf + *buf_pos, classes[i].chars, chars_len);
			*buf_pos += chars_len;
			return (classes[i].plen);
		}
		i++;
	}
	return (0);
}

/* Test whether character `c` is in the bracket expression's char_set. A linear
   scan is fine here -- bracket classes are small (at most a few dozen chars
   after range expansion), and this runs once per filename character per bracket
   token, not in an inner loop. BRACKET_NEGATED flips the result so [^abc]
   matches anything except a, b, and c.

   The fold under `shopt -s nocaseglob` has to happen here as well as in the
   literal comparison, and negation is why it is not optional: with the
   option on, [^a] must reject `A` too. A version that folded only literals
   would answer `*.TXT` correctly and `[a-z]*` wrongly, in the same
   pattern. */
bool	glob_char_in_class(char c, t_glob *bracket)
{
	int		i;
	bool	found;

	if (!bracket->char_set || bracket->char_set_len == 0)
		return (false);
	found = false;
	i = 0;
	while (i < bracket->char_set_len)
	{
		if (bracket->char_set[i] == c || (glob_nocase()
				&& ft_tolower((unsigned char)bracket->char_set[i])
				== ft_tolower((unsigned char)c)))
		{
			found = true;
			break ;
		}
		i++;
	}
	if (bracket->flags & BRACKET_NEGATED)
		return (!found);
	return (found);
}

/* A multibyte character against the bracket: its byte sequence has to
   appear as such in the raw bracket text -- `[é]`, `[aé]`, `[^é]` -- which
   is membership by explicit member.  A range with multibyte endpoints is
   not resolved (bash orders those by collation, a different problem), so
   `[à-ü]` matches none of them rather than the wrong ones.  Negation flips
   the answer like the single-byte case. */
bool	glob_mb_in_class(const char *c, size_t n, t_glob *bracket)
{
	int		i;
	bool	found;

	found = false;
	i = 0;
	while (i + (int)n <= bracket->len)
	{
		if (ft_memcmp(bracket->start + i, c, n) == 0)
		{
			found = true;
			break ;
		}
		i++;
	}
	if (bracket->flags & BRACKET_NEGATED)
		return (!found);
	return (found);
}
