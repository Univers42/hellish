/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var_name.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:25:44 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:25:44 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdbool.h>
#include <ctype.h>

/* POSIX says a variable name starts with [a-zA-Z_]. The unsigned char cast
   is required by the C standard: isalpha on a plain char would be UB for
   chars with the high bit set (e.g. UTF-8 bytes). */
bool	is_var_name_p1(char c)
{
	return (isalpha((unsigned char)c) || c == '_');
}

/* Subsequent characters extend to [a-zA-Z0-9_]. Same unsigned cast for the
   same reason. The two-function split (p1 / p2) lets callers cheaply detect
   whether they're looking at a valid identifier start vs continuation without
   duplicating the is-digit check. */
bool	is_var_name_p2(char c)
{
	return (isalnum((unsigned char)c) || c == '_');
}

/* Advance past a quoted region or a backslash escape starting at s[i],
   so paren-matching scans (the $(...) boundary hunt in both the reparser
   and the expander) never count parens that live inside quotes — a sed
   script like 's,[-(],,' must not derail the depth. Inside double quotes
   a backslash still escapes the next character. Returns the index after
   the skipped span, or i unchanged when s[i] starts no quoted span. */
int	sh_skip_quoted(const char *s, int len, int i)
{
	char	q;

	if (s[i] == '\\' && i + 1 < len)
		return (i + 2);
	if (s[i] != '\'' && s[i] != '"')
		return (i);
	q = s[i++];
	while (i < len && s[i] != q)
	{
		if (q == '"' && s[i] == '\\' && i + 1 < len)
			i++;
		i++;
	}
	if (i < len)
		i++;
	return (i);
}
