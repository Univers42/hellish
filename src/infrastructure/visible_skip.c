/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   visible_skip.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 18:54:20 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 05:14:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include <stdbool.h>
#include <stddef.h>

size_t	skip_ansi_marker(const char *s, size_t i);
size_t	skip_ansi_escape(const char *s, size_t i);
size_t	skip_osc(const char *s, size_t i);

/* Advance past anything that occupies no columns: a \001..\002 guarded
   region, a raw CSI, or a raw OSC.  True when *i moved. */
bool	skip_invisible(const char *s, size_t *i)
{
	size_t	was;

	was = *i;
	if (s[*i] == '\001')
		*i = skip_ansi_marker(s, *i);
	else if ((unsigned char)s[*i] == 0x1b && s[*i + 1] == '[')
		*i = skip_ansi_escape(s, *i);
	else if ((unsigned char)s[*i] == 0x1b && s[*i + 1] == ']')
		*i = skip_osc(s, *i);
	return (*i != was);
}
