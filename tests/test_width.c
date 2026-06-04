/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_width.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <locale.h>
#include <wchar.h>

int	main(void)
{
	setlocale(LC_ALL, "");
	printf("U+1F9D1 person   : %d\n", wcwidth(0x1F9D1));
	printf("U+1F4C2 folder   : %d\n", wcwidth(0x1F4C2));
	printf("U+0301 combining : %d\n", wcwidth(0x0301));
	printf("U+1F40D snake    : %d\n", wcwidth(0x1F40D));
	printf("U+1F570 clock    : %d\n", wcwidth(0x1F570));
	printf("U+FE0F vs16      : %d\n", wcwidth(0xFE0F));
	return (0);
}
