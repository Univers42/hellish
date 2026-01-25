/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strcoll.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25 20:54:32 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/25 21:18:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

int				is_letter(unsigned char c);
int				is_digit_char(unsigned char c);
int				is_alnum_char(unsigned char c);
unsigned char	to_lower(unsigned char c);

static void	skip_non_alnum(const char **p1, const char **p2)
{
	while (**p1 && !is_alnum_char((unsigned char)**p1))
		(*p1)++;
	while (**p2 && !is_alnum_char((unsigned char)**p2))
		(*p2)++;
}

static int	compare_alnum_chars(const char **p1, const char **p2)
{
	int	c1;
	int	c2;

	c1 = to_lower((unsigned char)**p1);
	c2 = to_lower((unsigned char)**p2);
	if (c1 != c2)
		return (c1 - c2);
	(*p1)++;
	(*p2)++;
	return (0);
}

static int	handle_remaining_chars(const char *p1, const char *p2)
{
	while (*p1 && !is_alnum_char((unsigned char)*p1))
		p1++;
	while (*p2 && !is_alnum_char((unsigned char)*p2))
		p2++;
	if (*p1 || *p2)
		return (to_lower((unsigned char)*p1) - to_lower((unsigned char)*p2));
	return (0);
}

int	ft_strcoll(const char *s1, const char *s2)
{
	const char	*p1;
	const char	*p2;
	int			result;

	p1 = s1;
	p2 = s2;
	while (*p1 && *p2)
	{
		skip_non_alnum(&p1, &p2);
		if (!*p1 || !*p2)
			break ;
		result = compare_alnum_chars(&p1, &p2);
		if (result != 0)
			return (result);
	}
	result = handle_remaining_chars(p1, p2);
	if (result != 0)
		return (result);
	return (ft_strcmp(s1, s2));
}
