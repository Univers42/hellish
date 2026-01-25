/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25 20:53:15 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/25 20:53:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/*
** Locale-aware string comparison (simplified UTF-8 collation)
** In UTF-8 locales: case-insensitive primary sort, ignoring non-alnum chars
** This matches bash's sorting behavior with en_US.UTF-8
*/
int	is_letter(unsigned char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int	is_digit_char(unsigned char c)
{
	return (c >= '0' && c <= '9');
}

int	is_alnum_char(unsigned char c)
{
	return (is_letter(c) || is_digit_char(c));
}

unsigned char	to_lower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return (c + 32);
	return (c);
}
