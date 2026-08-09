/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_path.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/09 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/09 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Decode one character: byte length into *len, return its column width.
   A byte that does not decode is one opaque byte of width 1, and a
   non-printing character contributes 0 -- the same fallbacks
   measure_width() uses, so the two always agree on a given string. */
static int	char_cols(const char *p, int *len)
{
	mbstate_t	st;
	wchar_t		wc;
	size_t		n;
	int			w;

	ft_memset(&st, 0, sizeof(st));
	n = mbrtowc(&wc, p, MB_CUR_MAX, &st);
	if (n == (size_t) - 1 || n == (size_t) - 2 || n == 0)
	{
		*len = 1;
		return (1);
	}
	*len = (int)n;
	w = wcwidth(wc);
	if (w < 0)
		return (0);
	return (w);
}

/* Copy of the tail of `path` occupying at most `cols` terminal columns,
   always starting ON a character boundary.

   The caller used to cut with `path + strlen(path) - n`, a raw BYTE offset
   against a COLUMN budget. Both halves are wrong once a path holds one
   non-ASCII character: bytes over-count columns (n-tilde is 2 bytes, 1
   column) so the prompt shrank further than asked, and the offset can land
   INSIDE a UTF-8 sequence -- the tail then opens with a continuation byte
   the terminal renders as a replacement glyph of its own choosing. That is
   how a prompt silently ends up a column or two off from the width the
   layout computed for it. Dropping whole characters from the front cannot
   land mid-sequence. */
char	*path_tail_cols(const char *path, int cols)
{
	const char	*p;
	int			w;
	int			len;

	if (cols < 1)
		return (ft_strdup(""));
	p = path;
	w = measure_width(path);
	while (*p && w > cols)
	{
		w -= char_cols(p, &len);
		p += len;
	}
	return (ft_strdup(p));
}
