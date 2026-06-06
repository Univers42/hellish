/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ifs_split.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* IFS whitespace: a character that is BOTH in IFS and is a whitespace
   char (space, tab, newline).  POSIX treats IFS whitespace specially:
   multiple adjacent IFS-whitespace chars collapse into a single delimiter,
   while non-whitespace IFS chars produce empty fields between them. */
bool	is_ws_ifs(char c, const char *ifs)
{
	return (ft_strchr(ifs, c) != NULL && (c == ' ' || c == '\t' || c == '\n'));
}

/* IFS non-whitespace: in IFS but NOT space/tab/newline.  Examples: IFS=:
   or IFS=,  Each occurrence produces exactly one field boundary, even if
   adjacent — so "a::b" with IFS=: yields three fields: a, "", b. */
bool	is_nw_ifs(char c, const char *ifs)
{
	return (ft_strchr(ifs, c) != NULL && c != ' ' && c != '\t' && c != '\n');
}

/* Return true if IFS contains any non-whitespace delimiter character.
   Used to choose between the two splitting strategies: ws-only IFS strips
   leading/trailing blanks and collapses runs; mixed IFS preserves empty
   fields.  Pure whitespace IFS (the default " \t\n") returns false. */
bool	ifs_has_nonws(const char *ifs)
{
	while (*ifs)
		if (!is_ws_ifs(*ifs++, " \t\n"))
			return (true);
	return (false);
}

/* Copy s[start..end) into a fresh string. Avoids ft_substr, which re-measures
   the whole source with ft_strlen on every call (O(n^2) when splitting many
   fields out of one long value). */
void	push_f(t_vec *out, const char *s, size_t start, size_t end)
{
	char	*f;
	size_t	len;

	len = end - start;
	f = xmalloc(len + 1);
	if (!f)
		return ;
	ft_memcpy(f, s + start, len);
	f[len] = '\0';
	vec_push(out, &f);
}

/* After hitting a non-whitespace IFS delimiter, skip any following IFS
   whitespace chars (POSIX: "IFS whitespace adjacent to an IFS non-whitespace
   is treated as a single delimiter").  This handles IFS=": " splitting of
   "a : b" into "a", "b" rather than "a", "", "b". */
size_t	skip_ws_delimiter(const char *s, size_t n,
			const char *ifs, size_t i)
{
	while (i < n && is_ws_ifs(s[i], ifs))
		i++;
	if (i < n && is_nw_ifs(s[i], ifs))
		i++;
	return (i);
}
