/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_hash2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* `${x:#pat}` at the TOKEN level, because whether it filters ELEMENTS or a
** joined string depends on the quoting and only the token carries that:
**
**     a=(foo bar fig)
**     printf '<%s>' ${a:#f*}     ->  <bar>            filters elements
**     echo "[${a:#f*}]"          ->  []               joins, then filters
**
** Both are "the filter"; they are different answers. The flagged form
** `${(M)x:#pat}` goes through zf_hash_form instead, which knows its own
** array-ness from the flag list -- same rule, different entry.
**
** Only reached in the zsh dialect, and only for a body that actually has
** the `:#` shape after a name; everything else falls through untouched. */
bool	zsh_hash_token(t_shell *state, t_token *tt)
{
	t_zflags	f;
	char		*out;
	int			nl;

	if (!zsh_mode(state) || tt->len < 3)
		return (false);
	if (zh_find(tt->start, tt->len, &nl) < 0)
		return (false);
	f = (t_zflags){0};
	f.split = true;
	f.array = (tt->tt != TT_DQENVVAR);
	out = zsh_hash_op(state, tt->start, tt->len,
			(t_zhash){false, f.array});
	if (!out)
		return (false);
	zf_emit_value(state, &f, tt, out);
	return (true);
}
