/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tprint.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 23:49:03 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 00:21:02 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"
#include <stdio.h>
#include <stdlib.h>

void tprint(const char *cap)
{
	char *x;
	char *y;

	x = tgetstr(cap, NULL);
	printf("%s: ", cap);
	if (x)
	{
		y = x;
		while (*y)
		{
			if ((unsigned char)*y <= ' ' || (unsigned char)*y == 0177)
				printf("\\%0o", (unsigned char)*y);
			else
				putchar(*y);
			y++;
		}
		free(x);
	}
	else
		printf("none");
	putchar('\n');
}
