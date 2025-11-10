/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gobble_line.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/11 00:49:42 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:49:44 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"
#include <string.h>

static char *find_nl(char *p)
{
    while (*p && *p != '\n')
        p++;
    return (p);
}

static void expand(t_buffer *b, char **ae)
{
    char *tmp;
    char *old;

    old = b->beg;
    b->size *= 2;
    tmp = (char *)xrealloc(old, b->size + 1);
    b->ptr = (b->ptr - old) + tmp;
    *ae = (*ae - old) + tmp;
    b->beg = tmp;
}

static void compact(t_buffer *b, char **ae)
{
    *ae -= b->ptr - b->beg;
    memmove(b->beg, b->ptr, b->full);
    b->full -= b->ptr - b->beg;
    b->ptr = b->beg;
}

static void fill(int fd, t_buffer *b)
{
    int n;

    n = read(fd, b->beg + b->full, b->size - b->full);
    if (n <= 0)
        b->ateof = 1;
    else
    {
        b->full += n;
        b->beg[b->full] = '\0';
    }
}

char *gobble_line(int fd, t_buffer *b, char *ae)
{
    char *end;

    if (!ae)
        ae = b->ptr;
    while (1)
    {
        end = find_nl(ae);
        if (*end)
            break;
        if (b->ateof)
            return (b->beg + b->full);
        if (b->ptr == b->beg)
        {
            if (b->full == b->size)
                expand(b, &ae);
        }
        else
            compact(b, &ae);
        fill(fd, b);
    }
    return (end + 1);
}
