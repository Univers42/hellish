/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_json.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"

/* Write the escaped form of one char; return how many bytes were written. */
static int	esc_one(char *out, char c)
{
	if (c == '"' || c == '\\')
		return (out[0] = '\\', out[1] = c, 2);
	if (c == '\n')
		return (out[0] = '\\', out[1] = 'n', 2);
	if (c == '\t')
		return (out[0] = '\\', out[1] = 't', 2);
	if (c == '\r')
		return (out[0] = '\\', out[1] = 'r', 2);
	return (out[0] = c, 1);
}

/* Escape a string for use as a JSON value. Covers the characters that occur
   in shell text; ponytail: not a general encoder (control chars < 0x20 other
   than \n\t\r pass through -- fine for prompts). */
char	*ai_json_escape(const char *s)
{
	char	*out;
	size_t	i;
	size_t	j;

	out = xmalloc(ft_strlen(s) * 2 + 1);
	i = 0;
	j = 0;
	while (s[i])
		j += esc_one(out + j, s[i++]);
	out[j] = '\0';
	return (out);
}

/* Translate one escape (e points at the char after the backslash). */
static int	un_esc(char *out, const char *e)
{
	if (*e == 'n')
		return (*out = '\n', 2);
	if (*e == 't')
		return (*out = '\t', 2);
	if (*e == 'r')
		return (*out = '\r', 2);
	return (*out = *e, 2);
}

/* Copy a JSON string body up to the closing unescaped quote. */
static char	*unescape(const char *p)
{
	char	*out;
	size_t	j;

	out = xmalloc(ft_strlen(p) + 1);
	j = 0;
	while (*p && *p != '"')
	{
		if (*p == '\\' && p[1])
			p += un_esc(out + j++, p + 1);
		else
			out[j++] = *p++;
	}
	out[j] = '\0';
	return (out);
}

/* Find "key" and return its string value (xfree it), or NULL.
   ponytail: first match wins; \uXXXX passes through literally. */
char	*ai_json_get_str(const char *buf, const char *key)
{
	char	*k;
	char	*p;

	k = ft_strjoin("\"", key);
	p = ft_strnstr(buf, k, ft_strlen(buf));
	xfree(k);
	if (!p)
		return (NULL);
	p += ft_strlen(key) + 1;
	while (*p && *p != ':')
		p++;
	while (*p && *p != '"')
		p++;
	if (!*p)
		return (NULL);
	return (unescape(p + 1));
}
