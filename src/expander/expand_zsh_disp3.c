/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_disp3.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 09:35:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 09:35:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* The zsh forms that live at the TOKEN level -- the flag list and the :#
** filter -- evaluated from plain TEXT instead.
**
** They are token-level because both need to know whether the expansion is
** still an array, and only the token carries the quoting that decides it.
** A NESTED expansion has no token of its own: `${${(M)x:#/[*]}:-FB}` reaches
** the nesting path as a slice of the outer body, so the flag parser was
** never consulted and the inner `(M)...` came back as a bad substitution --
** while the identical text on its own worked.
**
** The synthetic token is TT_ENVVAR, i.e. unquoted: an inner expansion is
** evaluated before the outer operator sees it, and at that point nothing has
** joined it yet. zsh_strlen builds one the same way and for the same
** reason. */
char	*zsh_token_text(t_shell *state, const char *s, int slen)
{
	t_token	tok;

	if (!zsh_mode(state) || slen <= 0)
		return (NULL);
	tok = (t_token){.tt = TT_ENVVAR, .start = (char *)s, .len = slen};
	if (!expand_zsh_flags(state, &tok, false)
		&& !zsh_hash_token(state, &tok))
		return (NULL);
	if (tok.allocated)
		return ((char *)tok.start);
	return (ft_strndup(tok.start, (size_t)tok.len));
}
