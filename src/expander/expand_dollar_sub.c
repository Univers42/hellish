/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_dollar_sub.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:37 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:37 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Public entry: expand a $(...) or $((...)) at the start of `s` into `outbuf`,
   returning the number of input chars consumed (0 if not a substitution). */
int	expand_dollar_sub(t_shell *state, const char *s, int slen,
		t_string *outbuf)
{
	int				consumed;
	t_expand_ctx	ctx;

	consumed = 0;
	ctx = init_expand(s, slen, outbuf, &consumed);
	if (process_arith_sub(state, &ctx))
		return (consumed);
	if (process_cmd_sub(state, &ctx))
		return (consumed);
	return (0);
}
