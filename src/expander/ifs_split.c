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

static bool	is_ws_ifs(char c, const char *ifs)
{
	return (ft_strchr(ifs, c) != NULL && (c == ' ' || c == '\t' || c == '\n'));
}

static bool	is_nw_ifs(char c, const char *ifs)
{
	return (ft_strchr(ifs, c) != NULL && c != ' ' && c != '\t' && c != '\n');
}

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
static void	push_f(t_vec *out, const char *s, size_t start, size_t end)
{
	char	*f;
	size_t	len;

	len = end - start;
	f = malloc(len + 1);
	if (!f)
		return ;
	ft_memcpy(f, s + start, len);
	f[len] = '\0';
	vec_push(out, &f);
}

/* POSIX field splitting that preserves empty fields produced by IFS
   non-whitespace delimiters (e.g. IFS=, "1,2,,3" -> 1,2,"",3). A maximal run
   of IFS whitespace, optionally surrounding one non-whitespace delimiter, is a
   single delimiter; leading/trailing IFS whitespace is ignored. */
char	**ifs_split_posix(const char *s, const char *ifs)
{
	t_vec	out;
	size_t	i;
	size_t	start;
	size_t	n;

	vec_init(&out);
	out.elem_size = sizeof(char *);
	n = ft_strlen(s);
	i = 0;
	while (i < n && is_ws_ifs(s[i], ifs))
		i++;
	start = i;
	while (i < n)
	{
		if (is_nw_ifs(s[i], ifs))
			(push_f(&out, s, start, i), i++, start = i);
		else if (is_ws_ifs(s[i], ifs))
		{
			push_f(&out, s, start, i);
			while (i < n && is_ws_ifs(s[i], ifs))
				i++;
			if (i < n && is_nw_ifs(s[i], ifs))
				i++;
			start = i;
		}
		else
			i++;
	}
	if (start < n)
		push_f(&out, s, start, n);
	return (vec_push(&out, &(char *){NULL}), (char **)out.ctx);
}
