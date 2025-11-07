/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_wpunct.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 16:35:13 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 16:46:04 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_wctype.h"

int	ft_iswpunct(wint_t wc)
{
	if (ft_iswalpha(wc) || ft_iswdigit(wc))
		return (0);
	if (wc >= L'!' && wc <= L'~')
		return (1);
	return (0);
}
