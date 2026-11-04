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
#include "helpers.h"

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
   and the expander) never count parens that live inside quotes -- a sed
   script like 's,[-(],,' must not derail the depth. span_skip owns the
   walk, because a double-quoted span is not flat: its $(...), ${...} and
   `...` hold quotes of their own. Read flat, "$(tr -d '"')" ended at the
   inner `"`, the `'` after it opened a span with no end, and the whole
   outer substitution stayed in the word as literal text, with no error.
   An even count of quote bytes inside balanced by accident and hid it.
   Returns the index after the skipped span, or i unchanged when s[i]
   starts no quoted span. An unterminated span ends the scan at len. */
int	sh_skip_quoted(const char *s, int len, int i)
{
	int	end;

	if (s[i] == '\\' && i + 1 < len)
		return (i + 2);
	if (s[i] != '\'' && s[i] != '"')
		return (i);
	end = span_skip(s, i, len);
	if (end < 0)
		return (len);
	return (end);
}
