/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer_advance2.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:55 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

/* Skip a backslash-escaped character. If str[1] exists we step over both
   the backslash and the next char; otherwise we step over just the backslash.
   The second case (trailing backslash) shouldn't arise in valid input, but
   this is defensive against truncated strings. */
void	advance_bs(char **str)
{
	ft_assert(**str == '\\');
	if ((*str)[1])
		*str += 1;
	*str += 1;
}
