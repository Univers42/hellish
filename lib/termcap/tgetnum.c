/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tgetnum.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/11 00:50:14 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:50:20 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

int tc_atoi(const char *s)
{
    int n;
    int sign;

    n = 0;
    sign = 1;
    if (!s)
        return (0);
    if (*s == '-' || *s == '+')
    {
        if (*s == '-')
            sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (*s++ - '0');
    return (n * sign);
}

int tgetnum(const char *cap)
{
    t_tglob *g;
    char *ptr;

    g = access_glob();
    if (!g->term_entry || !cap)
        return (-1);
    ptr = find_capability(g->term_entry, (char *)cap);
    if (!ptr || ptr[-1] != '#')
        return (-1);
    return (tc_atoi(ptr));
}
