/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   eprintf.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 23:13:04 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 23:13:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ft_printf.h"

char	*ft_vaseprintf(const char *str, va_list args)
{
	int				i;
	t_fmt_spec		spec;
	t_buffer		buff;

	i = 0;
	writer_init(&buff);
	buff.no_write = 1;
	buff.out_fd = 2;
	while (str[i])
	{
		spec = parse(str, &i, args);
		print_spec(args, spec, &buff);
	}
	return (buff.buff);
}

char	*ft_aseprintf(const char *str, ...)
{
	va_list	args;
	char	*ret;

	va_start(args, str);
	ret = ft_vaseprintf(str, args);
	va_end(args);
	return (ret);
}

int	ft_veprintf(const char *str, va_list args)
{
	int				i;
	t_fmt_spec		spec;
	t_buffer		buff;

	i = 0;
	writer_init(&buff);
	buff.out_fd = 2;
	while (str[i])
	{
		spec = parse(str, &i, args);
		print_spec(args, spec, &buff);
	}
	if (buff.buff)
	{
		ft_eputmem(buff.buff, buff.len);
		free(buff.buff);
	}
	return (buff.total_put_in);
}

int	ft_eprintf(const char *str, ...)
{
	va_list	args;
	int		ret;

	va_start(args, str);
	ret = ft_veprintf(str, args);
	va_end(args);
	return (ret);
}
