/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_arith_sub_utils2.c                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 13:17:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 13:23:27 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

bool	is_double_open_paren_v1(int slen, const char *s, int j)
{
	return (s[j] == '(' && j + 1 < slen && s[j + 1] == '(');
}

bool	is_double_close_paren_v1(int slen, const char *s, int j)
{
	return (s[j] == ')' && j + 1 < slen && s[j + 1] == ')');
}

bool	is_single_open_paren(const char *s, int j)
{
	return (s[j] == '(');
}

bool	is_single_close_paren(const char *s, int j)
{
	return (s[j] == ')');
}

void	handle_double_open_paren(int *depth, int *j)
{
	*depth += 2;
	*j += 2;
}
