/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_prefix.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* `${!pre*}` and `${!pre@}` -- the NAMES of every set variable whose name
** begins with `pre`, not their values.
**
**     aa=1 ab=2 b=3; echo ${!a*}      bash: aa ab
**                                     hellish: ${!a*}: bad substitution
**
** and the message was the mild half: pf_bad_subst also calls exit_clean(127),
** so a script that so much as MENTIONED the form died on the spot.  It is how
** portable code asks "which of my variables are set", so the shape turns up
** in exactly the defensive prologues that are supposed to be safe to run.
**
** The two forms differ only in how they join, and only when quoted:
** `@` yields one field per name (like "$@"), `*` yields one joined string.
** Unquoted the distinction is invisible -- a variable NAME can never contain
** an IFS character, so splitting the joined form back apart is exact -- and
** that is why the `*` case can take the simple path in both contexts.
*/

/* Set (not merely declared) and matching the prefix. bash lists a name only
   when it has a value, which is why `declare -x LATER` does not show up. */
static bool	pfx_wanted(t_env *e, const char *pre, int nlen)
{
	if (!e->key || !e->value)
		return (false);
	return (ft_strncmp(e->key, (char *)pre, (size_t)nlen) == 0);
}

/* Every matching name, in collation order -- bash sorts this list, and a
   caller iterating it deserves a stable order across runs. */
static void	pfx_collect(t_shell *state, const char *pre, int nlen, t_vec *out)
{
	size_t	i;
	t_env	*e;
	char	*s;

	vec_init(out);
	out->elem_size = sizeof(char *);
	i = 0;
	while (i < state->env.len)
	{
		e = &((t_env *)state->env.ctx)[i++];
		if (pfx_wanted(e, pre, nlen))
		{
			s = ft_strdup(e->key);
			vec_push(out, &s);
		}
	}
	if (out->len > 1)
		ft_quicksort(out);
}

static void	pfx_free(t_vec *l)
{
	size_t	i;

	i = 0;
	while (i < l->len)
		xfree(((char **)l->ctx)[i++]);
	xfree(l->ctx);
}

static char	*pfx_join(t_vec *l)
{
	t_string	out;
	size_t		i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (i < l->len)
	{
		if (i > 0)
			vec_push_char(&out, ' ');
		vec_push_str(&out, ((char **)l->ctx)[i++]);
	}
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

/* Answer one ${!pre*} / ${!pre@}. `tt` holds the brace body, so the '!' is
   at [0] and the '*' or '@' at the end. The `@` form in a splitting context
   parks its fields on the marker registry -- the same mechanism ${!a[@]}
   and the zsh flag expansions use, so the fields are freed with the rest of
   the expansion even if it is abandoned halfway. */
bool	arr_prefix_names(t_shell *state, t_token *tt, bool split_ctx)
{
	t_vec	l;
	char	*enc;

	pfx_collect(state, tt->start + 1, tt->len - 2, &l);
	if (tt->start[tt->len - 1] == '@' && split_ctx && l.len > 0)
	{
		enc = arr_from_elems((char **)l.ctx, (int)l.len, NULL);
		pfx_free(&l);
		tt->start = arr_mark_push(state, enc, (int)ft_strlen(enc));
		tt->len = (int)ft_strlen(tt->start);
		tt->allocated = false;
		return (xfree(enc), true);
	}
	enc = pfx_join(&l);
	pfx_free(&l);
	return (arr_emit(tt, enc));
}
