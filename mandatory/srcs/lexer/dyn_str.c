/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dyn_str.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:35:53 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/19 17:36:35 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

static int  d_str_ensure_capacity(t_dynamic_str *d_str, size_t needed_capacity)
{
    char    *new_str;
    size_t  new_capacity;

    if (d_str->dyn_str_capacity >= needed_capacity)
        return (0);
    new_capacity = d_str->dyn_str_capacity;
    if (new_capacity == 0)
        new_capacity = 16;
    while (new_capacity < needed_capacity)
        new_capacity *= 2;
    new_str = malloc(new_capacity * sizeof(char));
    if (!new_str)
        return (-1);
    memcpy(new_str, d_str->str, d_str->len * sizeof(char));
    free(d_str->str);
    d_str->str = new_str;
    d_str->dyn_str_capacity = new_capacity;
    return (0);
}

int d_str_init(t_dynamic_str *d_str, size_t dyn_initial_capacity)
{
    if (!d_str)
        return (-1);
    if (dyn_initial_capacity == 0)
        dyn_initial_capacity = 8;
    d_str->str = malloc(dyn_initial_capacity * sizeof(char));
    if (!d_str->str)
    {
        d_str->dyn_str_capacity = 0;
        d_str->len = 0;
        return (-1);
    }
    d_str->dyn_str_capacity = dyn_initial_capacity;
    d_str->len = 0;
    d_str->str[0] = '\0';
    return (0);
}

int d_str_append_char(t_dynamic_str *d_str, char c)
{
    if (!d_str)
        return (-1);
    if (d_str_ensure_capacity(d_str, d_str->len + 2) != 0)
        return (-1);
    d_str->str[d_str->len] = c;
    d_str->len++;
    d_str->str[d_str->len] = '\0';
    return (0);
}

int d_str_append_str(t_dynamic_str *d_str, const char *str)
{
    size_t  len;

    if (!d_str || !str)
        return (-1);
    len = strlen(str);
    if (d_str_ensure_capacity(d_str, d_str->len + len + 1) != 0)
        return (-1);
    memcpy(d_str->str + d_str->len, str, len);
    d_str->len += len;
    d_str->str[d_str->len] = '\0';
    return (0);
}

void d_str_free(t_dynamic_str *d_str)
{
    if (!d_str)
        return ;
    free(d_str->str);
    d_str->str = NULL;
    d_str->len = 0;
    d_str->dyn_str_capacity = 0;
}

void d_str_reset(t_dynamic_str *d_str)
{
	if (!d_str || !d_str->str)
		return;
	d_str->len = 0;
	d_str->str[0] = '\0';
}
