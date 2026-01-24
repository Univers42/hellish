/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_arith_sub.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 12:54:20 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 20:10:17 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

// helper: finalize arithmetic substitution handling

static void	proc_arith(int slen, const char *s, int *j, int *depth)
{
	if (is_double_open_paren_v1(slen, s, *j))
		return (handle_double_open_paren(depth, j));
	if (is_double_close_paren_v1(slen, s, *j))
		return (handle_double_close_paren(depth, j));
	if (is_single_open_paren(s, *j))
		return (handle_single_open_paren(depth, j));
	if (is_single_close_paren(s, *j))
		return (handle_single_close_paren(depth, j));
	(*j)++;
}

bool	process_arith_sub(t_shell *state, t_expand_ctx *ctx)
{
	const char	*s = ctx->s;
	int			slen;
	int			depth;
	int			j;

	slen = ctx->slen;
	if (!ctx || !s || !ctx->outbuf || !ctx->consumed)
		return (false);
	depth = 2;
	j = 3;
	if (slen < 4 || s[0] != IS_DOLLAR || s[1] != LPAREN || s[2] != LPAREN)
		return (false);
	while (j < slen && depth > 0)
		proc_arith(slen, s, &j, &depth);
	if (depth != 0)
		return (false);
	return (finish_arith_sub(state, ctx, j));
}
