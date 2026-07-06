/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_envvar2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:29:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 10:20:52 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Advance rp->i by one "logical unit" inside a ${...} while tracking the
   nesting depth. Quotes swallow their content so inner braces don't count
   (e.g. ${"}" is the variable named "}"). An opening brace bumps depth; a
   closing brace that hits depth==0 stops the scan (caller's loop exits on
   the next iteration). Everything else is consumed one character at a time. */
static void	scan_brace_depth(t_reparser *rp, int *depth)
{
	char	c;

	c = rp->current_token.start[rp->i];
	if (c == '\'' || c == '"')
	{
		skip_quoted_in_brace(rp, c);
		rp->i++;
	}
	else if (c == '{')
	{
		(*depth)++;
		rp->i++;
	}
	else if (c == '}' && --(*depth) == 0)
		return ;
	else
		rp->i++;
}

/* Handle ${...} variable expansion. Consume the '{', then scan until the
   matching '}' (depth-tracked, quote-aware), then push the interior as a
   subtoken. Returns false immediately if the character after $ is not '{',
   letting the caller fall through to the ident/special path. The final
   rp->i++ skips the closing '}' so the main loop continues past it.
   An empty ${} is pushed WITH its braces ("{}"): a zero-length subtoken
   would be indistinguishable from a bare '$' downstream and get emitted as
   a literal, while bash treats ${} as a fatal bad substitution — the brace
   payload fails pf_valid_plain and routes there. */
static bool	handle_envvar_brace(t_reparser *rp, t_tt tt)
{
	int	start;
	int	depth;

	if (rp->i >= rp->current_token.len
		|| rp->current_token.start[rp->i] != '{')
		return (false);
	rp->i++;
	start = rp->i;
	depth = 1;
	while (rp->i < rp->current_token.len && depth > 0)
		scan_brace_depth(rp, &depth);
	if (rp->i == start && rp->i < rp->current_token.len)
		push_subtoken_node(&rp->current_node, rp->current_token,
			create_interval(start - 1, rp->i + 1), tt);
	else
		push_subtoken_node(&rp->current_node, rp->current_token,
			create_interval(start, rp->i), tt);
	if (rp->i < rp->current_token.len)
		rp->i++;
	return (true);
}

/* Parse a $... expansion: consume the '$' then dispatch on the next char.
   Priority: ${brace} > $(paren)/special > plain identifier > lone-$ literal.
   The lone-$ case (prev_start == rp.i after consume_ident_rp) means the '$'
   was not followed by anything we recognise as an expansion; we call
   handle_envvar_empty which emits the '$' as a literal and optionally
   re-delegates to a quote parser if the very next char is a quote. */
void	reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt)
{
	t_reparser	rp;
	int			prev_start;

	ft_assert(t.start[(*i)++] == '$');
	create_reparser(&rp, *ret, t, i);
	prev_start = rp.i;
	if (handle_envvar_brace(&rp, tt))
		return (update_envvar_result(ret, i, &rp));
	if (handle_envvar_paren_or_special(&rp, prev_start, tt))
		return (update_envvar_result(ret, i, &rp));
	consume_ident_rp(&rp);
	if (prev_start == rp.i)
	{
		if (handle_envvar_empty(&rp, prev_start, tt))
			return (update_envvar_result(ret, i, &rp));
	}
	else
		handle_envvar_ident(&rp, prev_start, tt);
	update_envvar_result(ret, i, &rp);
}
