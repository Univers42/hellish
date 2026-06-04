/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_arith_sub_utils2b.c                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 13:16:56 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 13:22:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

char	*arith_expand(t_shell *state, const char *expr, int len);

bool	finish_arith_sub(t_shell *state, t_expand_ctx *ctx, int j)
{
	char	*result;
	char	*expr;
	t_token	tmp;

	tmp.start = ft_strndup(ctx->s + 3, (j - 2) - 3);
	tmp.len = (j - 2) - 3;
	tmp.tt = TT_WORD;
	tmp.allocated = true;
	process_word_token(state, &tmp);
	if (ft_strchr(tmp.start, '$'))
	{
		expr = expand_arith_vars(state, tmp.start, tmp.len);
		result = arith_expand(state, expr, (int)ft_strlen(expr));
		free(expr);
	}
	else
		result = arith_expand(state, tmp.start, tmp.len);
	free(tmp.start);
	if (result)
	{
		vec_push_nstr(ctx->outbuf, result, ft_strlen(result));
		free(result);
	}
	*ctx->consumed = j;
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
