/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_emit.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:35:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:35:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "parena.h"
#include "env.h"

/* Install an owned scalar as the token's expansion result. */
void	zf_install(t_token *tt, char *owned)
{
	tt->start = owned;
	tt->len = (int)ft_strlen(owned);
	tt->allocated = true;
	parena_note_attach();
}

/* Hand the splitter a list of fields.
**
** The existing deferral registry does this for ${arr[@]} by parking the
** array's NAME and letting the splitter look it up again.  A flagged
** expansion has no name -- `${(f)$(git branch)}` is computed -- so the
** marker carries the ENCODED VALUE instead, and the leading ARR_MAGIC is
** what tells the splitter which of the two it is holding.  That byte can
** never begin a variable name, so the two kinds cannot be confused, and it
** is the same trick the '!' prefix already plays for ${!a[@]}.
**
** Reusing the registry rather than adding a second mechanism matters for
** ownership: markers are freed wholesale at the start of the next
** simple-command expansion, so an expansion abandoned halfway -- a syntax
** error, a failing command substitution -- cannot leak the list.
*/
static void	zf_mark(t_shell *state, t_token *tt, t_vec *l)
{
	char	*enc;

	enc = arr_from_elems((char **)l->ctx, (int)l->len, NULL);
	zl_free(l);
	tt->start = arr_mark_push(state, enc, (int)ft_strlen(enc));
	tt->len = (int)ft_strlen(tt->start);
	tt->allocated = false;
	xfree(enc);
}

/* Does this expansion produce separate words, or one joined string?
**
** zsh, unlike bash, does NOT decide this by IFS -- it decides by whether
** the expansion is an array and whether it is quoted.  `${(f)x}` unquoted
** is a list of words; `"${(f)x}"` is one word with the fields joined,
** which is why plugin authors write `"${(@f)x}"` when they want both the
** quoting and the fields.  Getting this backwards is silent: the loop
** still runs, just once, over a string that looks almost right.
**
** split_ctx is the veto -- inside [[ ]], a here-string, or a scalar
** assignment there is no such thing as a second field, so the join is the
** only representable answer.
*/
static bool	zf_wants_fields(t_zflags *f, t_vec *l)
{
	if (l->len < 2 || !f->split || !f->array)
		return (false);
	return (!zf_has(f, 'j') && !zf_has(f, 'F'));
}

/* An UNQUOTED array expansion drops its empty elements; `"${(@f)x}"` keeps
   them.  So `x=$'a\n\nb'` is two fields as ${(f)x} and three as "${(@f)x}",
   and a plugin counting lines gets a different answer depending on which it
   wrote.  This is zsh, not an accident of ours: bash has no equivalent rule,
   which is why it only shows up once a real zsh is the oracle. */
static void	zf_drop_empties(t_zflags *f, t_vec *l)
{
	size_t	i;

	if (!f->array || zf_has(f, '@'))
		return ;
	i = 0;
	while (i < l->len)
	{
		if (!((char **)l->ctx)[i][0])
			zl_erase(l, i);
		else
			i++;
	}
}

/* Turn the finished list back into what the token holds.  (j) and (F) are
   explicit joins and win over everything; otherwise a multi-field result
   either defers to the splitter or joins with a space, and a single field
   is just itself.  Consumes the list either way. */
void	zf_emit(t_shell *state, t_zflags *f, t_token *tt, t_vec *l)
{
	char	*out;

	if (!zf_has(f, 'j') && !zf_has(f, 'F'))
		zf_drop_empties(f, l);
	if (zf_wants_fields(f, l))
		return (zf_mark(state, tt, l));
	if (zf_has(f, 'j'))
		out = zl_join(l, f->join);
	else if (zf_has(f, 'F'))
		out = zl_join(l, "\n");
	else
		out = zl_join(l, " ");
	zl_free(l);
	zf_install(tt, out);
}
