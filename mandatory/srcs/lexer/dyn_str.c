/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dyn_str.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:35:53 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/22 20:42:01 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "inlexer.h"

static int	d_str_ensure_capacity(t_d_str *d_str, size_t needed_capacity)
{
	char	*new_str;
	size_t	new_capacity;

	if (d_str->d_str_cap >= needed_capacity)
		return (0);
	new_capacity = d_str->d_str_cap;
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
	d_str->d_str_cap = new_capacity;
	return (0);
}

int	d_str_init(t_d_str *d_str, size_t dyn_initial_capacity)
{
	if (!d_str)
		return (-1);
	if (dyn_initial_capacity == 0)
		dyn_initial_capacity = 1024;
	d_str->str = malloc(dyn_initial_capacity * sizeof(char));
	if (!d_str->str)
	{
		d_str->d_str_cap = 0;
		d_str->len = 0;
		return (-1);
	}
	d_str->d_str_cap = dyn_initial_capacity;
	d_str->len = 0;
	d_str->str[0] = '\0';
	return (0);
}

int	d_str_append_char(t_d_str *d_str, char c)
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

int	d_str_append_str(t_d_str *d_str, const char *str)
{
	size_t	len;

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

void	d_str_free(t_d_str *d_str)
{
	if (!d_str)
		return ;
	free(d_str->str);
	d_str->str = NULL;
	d_str->len = 0;
	d_str->d_str_cap = 0;
}
