/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dyn_str_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 16:16:52 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/21 16:16:55 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

void	d_str_reset(t_d_str *d_str)
{
	if (!d_str || !d_str->str)
		return ;
	d_str->len = 0;
	d_str->str[0] = '\0';
	return ;
}
