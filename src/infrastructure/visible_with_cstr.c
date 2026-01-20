/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   visible_with_cstr.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 18:54:20 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 18:56:32 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

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
	w = ft_wcwidth(wc);
	if (w <= 0)
		w = 1;
	(*width) += w;
	return (i + consumed);
}

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
