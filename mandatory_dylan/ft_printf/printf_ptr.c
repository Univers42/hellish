/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_ptr.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 13:43:27 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 13:43:28 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdarg.h>
#include "ft_printf.h"

static void	handle_nullptr(t_fmt_spec spec, t_buffer *buff)
{
	int	total_len;

	total_len = 5;
	if (!(spec.flags & FL_MINUS) && spec.width != -1)
		writer_padn(buff, ' ', spec.width - total_len);
	writer_strn(buff, "(nil)", 5);
	if (spec.flags & FL_MINUS && spec.width != -1)
		writer_padn(buff, ' ', spec.width - total_len);
}

void	printf_ptr(va_list args, t_fmt_spec spec, t_buffer *buff)
{
	size_t	i;
	int		pad_len;
	int		total_len;
	int		digit_count;

	if (spec.t != FMT_POINTER)
		return ;
	i = va_arg(args, size_t);
	if (i == 0 && (handle_nullptr(spec, buff), 1))
		return ;
	spec.flags |= FL_POUND;
	spec = normalize_ptr_spec(spec);
	total_len = ptr_total_len(i, spec);
	pad_len = hex_padding_len(i, spec);
	digit_count = hex_digits(i);
	if (!(spec.flags & FL_MINUS) && spec.width != -1)
		writer_padn(buff, ' ', spec.width - total_len);
	if (spec.flags & FL_PLUS)
		writer_char(buff, '+');
	else if (spec.flags & FL_SPACE)
		writer_char(buff, ' ');
	writer_hex_lowcase(buff, i, spec.flags & FL_POUND, digit_count + pad_len);
	if (spec.flags & FL_MINUS && spec.width != -1)
		writer_padn(buff, ' ', spec.width - total_len);
}
