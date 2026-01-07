/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 13:43:37 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 14:38:53 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

void	print_spec(va_list args, t_fmt_spec spec, t_buffer *buff)
{
	if (spec.t == FMT_NORMAL_CHAR)
		writer_char(buff, spec.data);
	if (spec.t == FMT_PERCENT)
		writer_char(buff, '%');
	printf_char(args, spec, buff);
	printf_string(args, spec, buff);
	printf_int(args, spec, buff);
	printf_unsigned(args, spec, buff);
	printf_hex_lowcase(args, spec, buff);
	printf_hex_upcase(args, spec, buff);
	printf_ptr(args, spec, buff);
}

char	*ft_vasprintf(const char *str, va_list args)
{
	int				i;
	t_fmt_spec		spec;
	t_buffer		buff;

	i = 0;
	writer_init(&buff);
	buff.no_write = 1;
	while (str[i])
	{
		spec = parse(str, &i, args);
		print_spec(args, spec, &buff);
	}
	return (buff.buff);
}

char	*ft_asprintf(const char *str, ...)
{
	va_list	args;
	char	*ret;

	va_start(args, str);
	ret = ft_vasprintf(str, args);
	va_end(args);
	return (ret);
}

int	ft_vprintf(const char *str, va_list args)
{
	int				i;
	t_fmt_spec		spec;
	t_buffer		buff;

	i = 0;
	writer_init(&buff);
	while (str[i])
	{
		spec = parse(str, &i, args);
		print_spec(args, spec, &buff);
	}
	if (buff.buff)
	{
		ft_putmem(buff.buff, buff.len);
		free(buff.buff);
	}
	return (buff.total_put_in);
}

int	ft_printf(const char *str, ...)
{
	va_list	args;
	int		ret;

	va_start(args, str);
	ret = ft_vprintf(str, args);
	va_end(args);
	return (ret);
}
