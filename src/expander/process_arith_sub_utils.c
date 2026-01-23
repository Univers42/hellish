/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_arith_sub_utils.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 13:16:56 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 13:22:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

bool	finish_arith_sub(t_shell *state, t_expand_ctx *ctx, int j)
{
	const char	*s = ctx->s;
	t_string	*outbuf;
	int			*consumed;
	int			expr_len;
	char		*result;

	outbuf = ctx->outbuf;
	consumed = ctx->consumed;
	expr_len = (j - 2) - 3;
	result = arith_expand(state, (char *)s + 3, expr_len);
	if (result)
		(vec_push_nstr(outbuf, result, ft_strlen(result)), free(result));
	*consumed = j;
	return (true);
}

void	handle_double_close_paren(int *depth, int *j)
{
	*depth -= 2;
	*j += 2;
}

void	handle_single_open_paren(int *depth, int *j)
{
	(*depth)++;
	(*j)++;
}

void	handle_single_close_paren(int *depth, int *j)
{
	(*depth)--;
	(*j)++;
}
