/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_int.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 13:43:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 13:43:23 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdarg.h>
#include "ft_printf.h"

void	printf_int(va_list args, t_fmt_spec spec, t_buffer *buff)
{
	int	i;
	int	pad_len;
	int	total_len;
	int	digits;

	if (spec.t != FMT_INTEGER)
		return ;
	i = va_arg(args, int);
	spec = normilize_int_spec(spec, i);
	total_len = signed_total_len(i, spec);
	pad_len = signed_padding_len(i, spec);
	digits = signed_num_digits(i);
	if (!(spec.flags & FL_MINUS) && spec.width != -1)
		writer_padn(buff, ' ', spec.width - total_len);
	if (i >= 0 && !(spec.flags & FL_PLUS) && spec.flags & FL_SPACE)
		writer_char(buff, ' ');
	writer_signed(buff, i, spec.flags & FL_PLUS, pad_len + digits);
	if (spec.flags & FL_MINUS && spec.width != -1)
		writer_padn(buff, ' ', spec.width - total_len);
}

void	printf_unsigned(va_list args, t_fmt_spec spec, t_buffer *buff)
{
	unsigned int	i;
	int				pad_len;
	int				total_len;
	int				digits;

	if (spec.t != FMT_UNSIGNED)
		return ;
	i = va_arg(args, unsigned int);
	spec.flags &= ~FL_PLUS;
	spec.flags &= ~FL_SPACE;
	spec = normilize_int_spec(spec, i);
	total_len = signed_total_len(i, spec);
	pad_len = signed_padding_len(i, spec);
	digits = signed_num_digits(i);
	if (spec.flags & FL_MINUS)
		writer_signed(buff, i, spec.flags & FL_PLUS, digits + pad_len);
	if (spec.width != -1)
		writer_padn(buff, ' ', spec.width - total_len);
	if (!(spec.flags & FL_MINUS))
		writer_signed(buff, i, spec.flags & FL_PLUS, digits + pad_len);
}
