/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   writer.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 13:43:56 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 13:43:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"
#include <unistd.h>
#include <stdlib.h>
#include "../utils/utils.h"

int	writer_buffer_double(t_buffer *ret)
{
	int			i;
	char		*buff;

	ret->cap = ret->cap * 2;
	buff = malloc(ret->cap + 1);
	if (buff == 0)
		return (1);
	ret->buff[ret->len] = 0;
	i = -1;
	while (++i < ret->len)
		buff[i] = ret->buff[i];
	free(ret->buff);
	ret->buff = buff;
	return (0);
}

void	writer_char(t_buffer *buffer, char c)
{
	buffer->total_put_in++;
	if (buffer->len == buffer->cap)
	{
		if (writer_buffer_double(buffer))
		{
			if (!buffer->no_write)
				ft_putmem(buffer->buff, buffer->len);
			free(buffer->buff);
			buffer->buff = 0;
		}
	}
	if (buffer->buff)
	{
		buffer->buff[buffer->len++] = c;
		buffer->buff[buffer->len] = 0;
	}
	else if (!buffer->no_write)
		write(buffer->out_fd, &c, 1);
}

void	writer_padn(t_buffer *buff, char padding, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		writer_char(buff, padding);
		n--;
	}
}

void	writer_strn(t_buffer *buff, char *str, int n)
{
	int	i;

	i = 0;
	while (i < n && str[i])
	{
		writer_char(buff, str[i]);
		i++;
	}
}

void	writer_signed(t_buffer *buff, ssize_t n, int add_sign, int digits)
{
	if (n < 0)
		writer_char(buff, '-');
	else if (add_sign)
		writer_char(buff, '+');
	if (n / 10 || digits > 0)
		writer_signed(buff, ft_abs(n / 10), 0, digits - 1);
	if (digits > 0)
		writer_char(buff, ft_abs(n % 10) + '0');
}
