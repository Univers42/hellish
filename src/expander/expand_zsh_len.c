/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_len.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 19:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 19:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* ${#${(f)x}} -- how many FIELDS the flagged expansion produced, not how
** many bytes the string it would have printed is long.
**
** `x=$'a\nb\nc'` gives 3 here and 5 for the joined "a b c", and counting
** lines is most of what (f) is used for, so the wrong one of those is both
** plausible and useless.  zsh answers 3 even inside double quotes -- the
** operand of `#` is an array whatever the quoting around it -- which is why
** the synthetic token below is TT_ENVVAR rather than copying the real one.
**
** Returns NULL when the operand is not a flagged expansion, so expand_strlen
** carries on to the ordinary ${#name} path untouched.
*/
char	*zsh_strlen(t_shell *state, const char *s, int slen)
{
	t_token	tok;
	char	*v;
	char	*r;

	if (!zsh_mode(state) || !zf_is_nested(s, slen))
		return (NULL);
	tok = (t_token){.tt = TT_ENVVAR, .start = (char *)s, .len = slen};
	v = zf_nested(state, &tok, s, slen);
	if (!v)
		return (NULL);
	if (arr_is(v))
		r = ft_itoa(arr_count(v));
	else
		r = ft_itoa((int)ft_strlen(v));
	xfree(v);
	return (r);
}
