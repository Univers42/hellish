/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 23:56:24 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:58:41 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Called when a bare $ was not followed by any recognisable expansion form.
   First check whether the next char is a quote -- if so, hand off to the
   quote parser (handle_envvar_quote) and return true to signal "handled".
   If the next char was a quote that we consumed, we're done; otherwise call
   handle_envvar_literal to emit the '$' itself as a literal and return false
   so the caller knows no quote context was entered. */
bool	handle_envvar_empty(t_reparser *rp, int prev_start, t_tt tt)
{
	handle_envvar_quote(rp, prev_start, tt);
	if (prev_start < rp->current_token.len
		&& (rp->current_token.start[prev_start] == '\''
			|| rp->current_token.start[prev_start] == '"')
		&& tt != TT_DQENVVAR)
		return (true);
	handle_envvar_literal(rp, prev_start, tt);
	return (false);
}

/* Emit a lone '$' (and possibly the char after it) as a literal subtoken when
   the $ did not start any recognised expansion. Inside TT_DQENVVAR we push
   just the '$' character (prev_start-1 .. prev_start) as TT_DQWORD; outside,
   we use select_literal_tt to pick TT_SQWORD or TT_ENVVAR depending on what
   character follows -- the range is prev_start-1 .. rp->i which may include
   the following char if select_literal_tt decided it belongs here. */
void	handle_envvar_literal(t_reparser *rp, int prev_start, t_tt tt)
{
	t_tt	chosen;

	if (tt == TT_DQENVVAR)
	{
		push_subtoken_node(&rp->current_node, rp->current_token,
			create_interval(prev_start - 1, prev_start), TT_DQWORD);
		return ;
	}
	chosen = select_literal_tt(tt, &rp->current_token, prev_start);
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(prev_start - 1, rp->i), chosen);
}

/* Convenience wrappers that adapt the is_*_paren(token, idx) functions for
   callers that only have a t_reparser. Avoids spelling out rp->current_token
   and rp->i at every call site, which would bloat the dispatch loops. */
bool	is_double_close_paren_rp(t_reparser *rp)
{
	return (is_double_close_paren(rp->current_token, rp->i));
}

bool	is_open_paren_rp(t_reparser *rp)
{
	return (is_open_paren(rp->current_token, rp->i));
}

bool	is_close_paren_rp(t_reparser *rp)
{
	return (is_close_paren(rp->current_token, rp->i));
}
