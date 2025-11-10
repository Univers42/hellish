/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tgetflag.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/11 00:50:08 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:50:11 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

int tgetflag(const char *cap)
{
	t_tglob *g;
	char *ptr;

	g = access_glob();
	if (!g->term_entry || !cap)
		return (0);
	ptr = find_capability(g->term_entry, (char *)cap);
	return (ptr && ptr[-1] == ':');
}
