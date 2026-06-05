/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header5.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header_private.h"

/* Copy the ANSI escape at `src` verbatim (zero width) into the clip buffer at
   offset `o`, stopping at capacity. Returns the new offset. */
static size_t	put_ansi(char *dst, size_t o, size_t size, const char *src)
{
	int	n;
	int	i;

	n = skip_ansi(src);
	i = 0;
	while (i < n && o + 1 < size)
		dst[o++] = src[i++];
	return (o);
}

/* Copy one display character if it fits the column budget and the buffer.
   Returns the bytes consumed from `src`, or 0 to stop. */
static int	clip_char(t_clip *c, const char *src)
{
	wchar_t	wc;
	size_t	len;
	int		w;

	len = mbrtowc(&wc, src, MB_CUR_MAX, &c->st);
	if (len == (size_t) - 1 || len == (size_t) - 2 || len == 0)
		return (0);
	w = ft_max(wcwidth(wc), 0);
	if (c->used + w > c->max || c->o + len >= c->size)
		return (0);
	ft_memcpy(c->dst + c->o, src, len);
	c->o += len;
	c->used += w;
	return ((int)len);
}

/* Copy up to `max` display columns of `src` into `dst`; returns width used.
   ANSI colour escapes are copied through at zero width, so a truncated cell
   keeps its colours intact and never cuts an escape in half. */
int	header_clip(char *dst, size_t size, const char *src, int max)
{
	t_clip	c;
	int		esc;
	int		step;

	ft_memset(&c, 0, sizeof(c));
	c.dst = dst;
	c.size = size;
	c.max = max;
	while (*src && c.o + 1 < size)
	{
		esc = skip_ansi(src);
		if (esc)
		{
			c.o = put_ansi(dst, c.o, size, src);
			src += esc;
			continue ;
		}
		step = clip_char(&c, src);
		if (!step)
			break ;
		src += step;
	}
	dst[c.o] = '\0';
	return (c.used);
}
