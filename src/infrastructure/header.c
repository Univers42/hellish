/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   header.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:22 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"
#include "shell.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <sys/ioctl.h>

/* Unicode (UTF-8) is fine when LC_ALL / LC_CTYPE / LANG mention UTF-8. */
int	header_use_unicode(void)
{
	const char	*l;

	l = getenv("LC_ALL");
	if (!l || !*l)
		l = getenv("LC_CTYPE");
	if (!l || !*l)
		l = getenv("LANG");
	if (!l || !*l)
		return (0);
	if (ft_strnstr(l, "UTF-8", ft_strlen(l))
		|| ft_strnstr(l, "UTF8", ft_strlen(l))
		|| ft_strnstr(l, "utf-8", ft_strlen(l))
		|| ft_strnstr(l, "utf8", ft_strlen(l)))
		return (1);
	return (0);
}

/* The active glyph set: pretty box art on UTF-8, safe ASCII otherwise. */
t_glyphs	header_glyphs(void)
{
	static const t_glyphs	uni = {"\u256d", "\u256e", "\u2570", "\u256f",
		"\u2500", "\u2502", "\u203a", "\u00b7", "\u2026", "\u2b06"};
	static const t_glyphs	ascii = {"+", "+", "+", "+", "-", "|", ">", "-",
		"~", "^"};

	if (header_use_unicode())
		return (uni);
	return (ascii);
}

/* Terminal width, so a header can span the whole line. */
int	header_cols(void)
{
	struct winsize	ws;

	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
		return ((int)ws.ws_col);
	return (80);
}

/* Display width of a UTF-8 string in terminal columns (ANSI-free input). */
int	header_width(const char *s)
{
	mbstate_t	st;
	wchar_t		wc;
	size_t		len;
	int			total;
	int			w;

	ft_memset(&st, 0, sizeof(st));
	total = 0;
	while (*s)
	{
		len = mbrtowc(&wc, s, MB_CUR_MAX, &st);
		if (len == (size_t) - 1 || len == (size_t) - 2 || len == 0)
			break ;
		w = wcwidth(wc);
		if (w > 0)
			total += w;
		s += len;
	}
	return (total);
}

/* Copy up to `max` display columns of `src` into `dst`; returns width used. */
int	header_clip(char *dst, size_t size, const char *src, int max)
{
	mbstate_t	st;
	wchar_t		wc;
	size_t		len;
	size_t		o;
	int			used;

	ft_memset(&st, 0, sizeof(st));
	o = 0;
	used = 0;
	while (*src && o + 1 < size)
	{
		len = mbrtowc(&wc, src, MB_CUR_MAX, &st);
		if (len == (size_t) - 1 || len == (size_t) - 2 || len == 0)
			break ;
		if (used + ft_max(wcwidth(wc), 0) > max || o + len >= size)
			break ;
		ft_memcpy(dst + o, src, len);
		o += len;
		used += ft_max(wcwidth(wc), 0);
		src += len;
	}
	dst[o] = '\0';
	return (used);
}
