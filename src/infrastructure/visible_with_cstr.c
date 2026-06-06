/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   visible_with_cstr.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 18:54:20 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 05:14:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* Skip a \001...\002 bracket pair: the markers readline uses to flag zero-
   width sequences in the prompt string. readline needs to know the *visible*
   width so it can place the cursor correctly -- if you forget these wrappers,
   every ANSI colour code bloats the count and the cursor drifts right. */
static size_t	skip_ansi_marker(const char *s, size_t i)
{
	if (s[i] == '\001')
	{
		i++;
		while (s[i] && s[i] != '\002')
			i++;
		if (s[i])
			i++;
	}
	return (i);
}

/* Skip a raw ESC [ ... final-byte sequence that slipped through without
   \001/\002 markers (e.g. in debug output). Same intent: invisible bytes. */
static size_t	skip_ansi_escape(const char *s, size_t i)
{
	if ((unsigned char)s[i] == 0x1b && s[i + 1] == '[')
	{
		i += 2;
		while (s[i] && !(s[i] >= 'A' && s[i] <= 'z'))
			i++;
		if (s[i])
			i++;
	}
	return (i);
}

/* Decode one multibyte character with mbrtowc, add its terminal cell width
   (wcwidth), and return the new byte offset. A decode error is treated as a
   single opaque byte with width 1 (the safe fallback for broken locale data).
   wcwidth returns -1 for control characters; we silently treat those as 0. */
static size_t	handle_wide_char(const char *s, size_t i,
								mbstate_t *st, int *width)
{
	wchar_t	wc;
	size_t	consumed;
	int		w;

	consumed = mbrtowc(&wc, s + i, MB_CUR_MAX, st);
	if (consumed == (size_t)-1 || consumed == (size_t)-2 || consumed == 0)
	{
		(*width) += 1;
		ft_memset(st, 0, sizeof(*st));
		return (i + 1);
	}
	w = wcwidth(wc);
	if (w > 0)
		(*width) += w;
	return (i + consumed);
}

/* Return the number of terminal columns that `s` occupies when printed: ANSI
   escapes (raw or bracketed) cost zero; multibyte characters (e.g. CJK) may
   cost two. This is the number readline needs for its prompt-width argument so
   the cursor, history navigation, and line wrapping all stay accurate. */
int	visible_width_cstr(const char *s)
{
	mbstate_t	st;
	size_t		i;
	int			width;

	i = 0;
	width = 0;
	ft_memset(&st, 0, sizeof(st));
	while (s[i])
	{
		if (s[i] == '\001')
		{
			i = skip_ansi_marker(s, i);
			continue ;
		}
		if ((unsigned char)s[i] == 0x1b && s[i + 1] == '[')
		{
			i = skip_ansi_escape(s, i);
			continue ;
		}
		i = handle_wide_char(s, i, &st, &width);
	}
	return (width);
}
