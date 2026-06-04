/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_ft_width.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <wchar.h>

static int	is_combining(wchar_t wc)
{
	return ((wc >= 0x0300 && wc <= 0x036F)
		|| (wc >= 0x1AB0 && wc <= 0x1AFF));
}

static int	is_wide(wchar_t wc)
{
	return ((wc >= 0x1100 && wc <= 0x115F)
		|| (wc >= 0x2E80 && wc <= 0xA4CF)
		|| (wc >= 0xAC00 && wc <= 0xD7A3)
		|| (wc >= 0xFF01 && wc <= 0xFF60)
		|| (wc >= 0xFFE0 && wc <= 0xFFE6));
}

int	ft_wcwidth(wchar_t wc)
{
	if (wc >= 0x20 && wc <= 0x7E)
		return (1);
	if (wc < 0x20 || (wc >= 0x7F && wc <= 0x9F))
		return (-1);
	if (is_combining(wc))
		return (0);
	if (is_wide(wc))
		return (2);
	return (1);
}

int	main(void)
{
	printf("U+1F9D1 person   : %d\n", ft_wcwidth(0x1F9D1));
	printf("U+1F4C2 folder   : %d\n", ft_wcwidth(0x1F4C2));
	printf("U+0301 combining : %d\n", ft_wcwidth(0x0301));
	printf("U+1F40D snake    : %d\n", ft_wcwidth(0x1F40D));
	printf("U+0041 letter A  : %d\n", ft_wcwidth(0x0041));
	printf("U+FE0F vs16      : %d\n", ft_wcwidth(0xFE0F));
	return (0);
}
