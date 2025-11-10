/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tgetstr.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/11 00:50:28 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:50:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

char *tgetstr(const char *cap, char **area)
{
	t_tglob *g;
	char *ptr;

	g = access_glob();
	if (!g->term_entry || !cap)
		return (NULL);
	ptr = find_capability(g->term_entry, (char *)cap);
	if (!ptr || (ptr[-1] != '=' && ptr[-1] != '~'))
		return (NULL);
	return (tgetst1(ptr, area));
}