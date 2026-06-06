/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_envvar_utils.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 23:29:27 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* If rp->i points at '(' or '((', pack the context into a t_paren_ctx and
   delegate to reparse_envvar_paren, then return true. The caller can then
   skip the ident/special dispatch with a single bool check instead of
   repeating the is_open_paren / is_double_open_paren tests inline. */
bool	try_handle_paren_rp(t_reparser *rp, int prev_start, t_tt tt)
{
	t_paren_ctx	ctx;

	if (!is_double_open_paren(rp->current_token, rp->i)
		&& !is_open_paren(rp->current_token, rp->i))
		return (false);
	ctx.ret = &rp->current_node;
	ctx.i = &rp->i;
	ctx.t = rp->current_token;
	ctx.prev_start = prev_start;
	ctx.tt = tt;
	reparse_envvar_paren(ctx);
	return (true);
}

/* Thin wrapper: try to match a special $X variable ($?, $$, $!, $#, $@, $*,
   $-, $0-$9) via reparse_special_envvar; returns true if one was consumed.
   Separated from try_handle_paren_rp so the calling code reads as a clean
   priority chain rather than a single big if-else. */
bool	try_handle_special_rp(t_reparser *rp, t_tt tt)
{
	return (reparse_special_envvar(&rp->current_node,
			&rp->i, rp->current_token, tt));
}

/* Advance rp->i past a POSIX variable name ([a-zA-Z_][a-zA-Z0-9_]*). The
   first character uses is_var_name_p1 (no digits), the rest use p2. We stop
   as soon as a non-ident char appears -- the caller decides what to do with
   the remaining input. If rp->i does not move, the caller knows there was
   no plain identifier to consume (lone $ case). */
void	consume_ident_rp(t_reparser *rp)
{
	if (rp->i < rp->current_token.len
		&& is_var_name_p1(rp->current_token.start[rp->i]))
		rp->i++;
	while (rp->i < rp->current_token.len
		&& is_var_name_p2(rp->current_token.start[rp->i]))
		rp->i++;
}

/* Choose the token type for a literal span adjacent to a $ that turned out
   not to be an expansion (e.g. a bare "$" with nothing after it). Inside
   double-quotes (TT_DQENVVAR) literals are TT_DQWORD; if the context started
   on a single or double quote, use TT_ENVVAR (it will be treated as literal
   by the expander); otherwise fall back to TT_SQWORD for unquoted literals.
   The asymmetry exists because the expander checks the token type to decide
   whether to IFS-split -- getting this wrong produces extra field splits. */
t_tt	select_literal_tt(t_tt ctx_tt, t_token *t, int prev_start)
{
	t_tt	out;

	if (ctx_tt == TT_DQENVVAR && prev_start < t->len
		&& t->start[prev_start] == '"')
		return (TT_DQWORD);
	if (prev_start < t->len && (t->start[prev_start] == '\''
			|| t->start[prev_start] == '"'))
		return (TT_ENVVAR);
	if (ctx_tt == TT_DQENVVAR)
		out = TT_DQWORD;
	else
		out = TT_SQWORD;
	return (out);
}
