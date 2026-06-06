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

/* NOTE: ft_strcoll is no longer used by glob_sort (which switched to ft_strcmp
   for POSIX byte-order compliance). It is kept here in case other code needs
   a locale-flavoured comparison. The algorithm is: skip non-alphanumeric chars
   from both strings in lockstep, compare the next alnum pair case-insensitively
   repeat. Ties are broken by a final plain ft_strcmp on the originals. This
   gives a "natural sort" feel but is NOT what bash uses for glob ordering. */
static void	skip_non_alnum(const char **p1, const char **p2)
{
	while (**p1 && !is_alnum_char((unsigned char)**p1))
		(*p1)++;
	while (**p2 && !is_alnum_char((unsigned char)**p2))
		(*p2)++;
}

/* Compare the current alphanumeric characters (case-folded) from both strings
   and advance past them. Returns non-zero on a difference so the caller can
   stop; returns 0 and advances both pointers on a match. */
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

/* After one string runs out of alphanumeric characters, compare whatever
   is left (skipping leading non-alnum from both sides). If both are exhausted
   simultaneously they're equal at this level. Otherwise the one with remaining
   alnum chars wins. */
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

/* Case-insensitive collation with non-alnum skipping. The final ft_strcmp
   tie-break ensures that two strings which are identical modulo case and
   punctuation still produce a stable, deterministic order. */
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
