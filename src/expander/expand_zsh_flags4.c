/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_flags4.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* `${(M)x:#pat}` -- the (M) flag exists only to invert the :# filter, so the
   two are one construct and are handled together.  Without this pairing (M)
   would have to mean something on its own, and it does not: zsh defines it
   as "return the MATCH", which for :# is "keep the elements that match" and
   for anything else is a no-op nobody writes.
     Returns true when it handled the token. */
bool	zf_hash_form(t_shell *state, t_zflags *f, t_token *tt, int end)
{
	char	*out;
	int		nl;

	if (zh_find(tt->start + end, tt->len - end, &nl) < 0)
		return (false);
	if (!zf_check(state, f, tt))
		return (true);
	out = zsh_hash_op(state, tt->start + end, tt->len - end,
			(t_zhash){zf_has(f, 'M'), f->array});
	if (!out)
		return (false);
	zf_emit_value(state, f, tt, out);
	return (true);
}
