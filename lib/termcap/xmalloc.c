/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   xmalloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/11 00:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:29:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"
#include <stdlib.h>
#include <unistd.h>

static void memory_out(void)
{
    write(2, "virtual memory exhausted\n", 25);
    exit(1);
}

void *xmalloc(size_t size)
{
    void *ptr;

    ptr = malloc(size);
    if (!ptr)
        memory_out();
    return (ptr);
}

void *xrealloc(void *ptr, size_t size)
{
    void *new_ptr;

    new_ptr = realloc(ptr, size);
    if (!new_ptr)
        memory_out();
    return (new_ptr);
}

/* Singleton: one shared instance across all translation units */
t_tglob *access_glob(void)
{
    static int speeds_data[] = {
        0, 50, 75, 110, 135, 150, -2, -3, -6, -12,
        -18, -24, -48, -96, -192, -288, -384, -576, -1152};
    static char esctab_data[] = " \007\010  \033\014 \012 \015 \011 \013 ";
    static t_tglob g = {
        .pc = 0,
        .ospeed = 0,
        .speeds = speeds_data,
        .up = NULL,
        .bc = NULL,
        .tputs_baud_rate = 0,
        .esctab = esctab_data,
        .term_entry = NULL};
    return (&g);
}

static int g_used_fallback = 0;

int termcap_used_fallback(void)
{
    return (g_used_fallback);
}

void termcap_mark_fallback(void)
{
    g_used_fallback = 1;
}
