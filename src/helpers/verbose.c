/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   verbose.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 18:12:50 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 18:35:12 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"

#ifdef VERBOSE

void	verbose(int flag, const char *str, ...)
{
	va_list	args;

	va_start(args, str);
	claptrap(flag, str, args);
	va_end(args);
}

#else

void	verbose(int flag, const char *str, ...)
{
	(void)flag;
	(void)str;
}

#endif
