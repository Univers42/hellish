/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_bare.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 20:50:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 20:50:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* `${${(s: :)$(git version)}[3]}` -- a nested flagged expansion with NO
** flags of its own on the outside.
**
** It is easy to miss that this is a separate case.  expand_zsh_flags claims
** a body that begins with `(`, and this one begins with `$`; the flags are
** one level down.  oh-my-zsh's git plugin opens with precisely this line,
** so missing it left the most-used plugin in the corpus failing on line 3
** with every flag it needed already implemented.
**
** The outer braces contribute only two things: the subscript, and the
** quoting that decides whether the result is still an array.  Everything
** else is the inner expansion, so this borrows zf_emit rather than growing
** a second way to install a field list.
*/
bool	zsh_bare_nested(t_shell *state, t_token *tt, bool split_ctx)
{
	t_zflags	f;
	char		*v;
	t_vec		l;

	if (!zf_is_nested(tt->start, tt->len))
		return (false);
	v = zf_nested(state, tt, tt->start, tt->len);
	if (!v)
		return (false);
	v = zn_subscript(state, v, tt->start, tt->len);
	f = (t_zflags){0};
	f.split = split_ctx;
	f.array = tt->tt != TT_DQENVVAR;
	l = zl_from(state, &f, v);
	xfree(v);
	zf_emit(state, &f, tt, &l);
	return (true);
}
