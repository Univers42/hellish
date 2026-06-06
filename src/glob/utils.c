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

/* Character classification helpers for ft_strcoll. They accept unsigned char
   to avoid signed-char UB with high bytes. These are intentionally simpler
   than the ctype.h macros -- no locale dependency, no lookup table, just
   range checks. They feed into the ft_strcoll logic; glob_sort itself uses
   ft_strcmp (byte order) and doesn't call these directly anymore. */
int	is_letter(unsigned char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

/* Return 1 if `c` is an ASCII decimal digit. */
int	is_digit_char(unsigned char c)
{
	return (c >= '0' && c <= '9');
}

/* Return 1 if `c` is an ASCII letter or digit. */
int	is_alnum_char(unsigned char c)
{
	return (is_letter(c) || is_digit_char(c));
}

/* Fold uppercase ASCII to lowercase; other bytes pass through unchanged. */
unsigned char	to_lower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return (c + 32);
	return (c);
}
