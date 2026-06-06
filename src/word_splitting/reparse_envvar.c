/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_envvar.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:29:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 10:20:52 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Write the reparser's updated position and current node back to the caller's
   out-parameters. Every branch in reparse_envvar ends with this call so the
   caller's *i and *ret stay consistent no matter which path we took. */
void	update_envvar_result(t_ast_node *ret, int *i, t_reparser *rp)
{
	*i = rp->i;
	*ret = rp->current_node;
}

/* After the '$', check if the next character opens a $(...) or $((...))
   substitution or is one of the single-character specials ($?, $$, $!, $#,
   $@, $*, $-, $0-$9). Returns true if a sub-parser claimed the token so
   the caller knows it can skip the plain-ident path. */
bool	handle_envvar_paren_or_special(t_reparser *rp,
			int prev_start, t_tt tt)
{
	if (try_handle_paren_rp(rp, prev_start, tt))
		return (true);
	if (try_handle_special_rp(rp, tt))
		return (true);
	return (false);
}

/* When a bare $ is immediately followed by a quote (e.g. $'...' in some
   shells, or a parser artefact), delegate to the appropriate quote parser
   rather than treating the quote as a literal. The TT_DQENVVAR guard keeps
   this from firing inside a double-quoted context where $" is not valid. */
void	handle_envvar_quote(t_reparser *rp, int prev_start, t_tt tt)
{
	if (prev_start < rp->current_token.len
		&& rp->current_token.start[prev_start] == '\'' && tt != TT_DQENVVAR)
	{
		reparse_squote(&rp->current_node, &rp->i, rp->current_token);
		return ;
	}
	if (prev_start < rp->current_token.len
		&& rp->current_token.start[prev_start] == '"' && tt != TT_DQENVVAR)
	{
		reparse_dquote(&rp->current_node, &rp->i, rp->current_token);
		return ;
	}
}

/* Push a plain variable-name subtoken: the span [prev_start, rp->i) holds
   the identifier characters already consumed by consume_ident_rp(). The tt
   argument (TT_ENVVAR or TT_DQENVVAR) carries quoting context so the
   expander knows whether to IFS-split the expansion result. */
void	handle_envvar_ident(t_reparser *rp, int prev_start, t_tt tt)
{
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(prev_start, rp->i), tt);
}

/* Advance rp->i to the matching close quote `q` (leaving it on the quote),
   so braces inside a quoted segment of ${...} are not counted. */
void	skip_quoted_in_brace(t_reparser *rp, char q)
{
	const char	*s;
	int			len;

	s = rp->current_token.start;
	len = rp->current_token.len;
	rp->i++;
	while (rp->i < len && s[rp->i] != q)
	{
		if (q == '"' && s[rp->i] == '\\' && rp->i + 1 < len)
			rp->i++;
		rp->i++;
	}
}
