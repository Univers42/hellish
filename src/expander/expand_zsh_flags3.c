/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_flags3.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

int	ansic_simple(char c);

/* The implemented roster, and nothing else gets through.  Grouped by what
   they do to the value:
     f s z    split into a list        j F      join a list back into one
     k v      keys / values            @        keep the fields separate
     o O n u  order and de-duplicate   p        escapes in the s/j argument
     U L C    case                     q        shell-quote
     %        prompt escapes           P        the value names a parameter
   Deliberately absent, so they fail loudly rather than lie: M (return the
   match), A (array assignment), b, e, t, V, w, W, X, ~, #.  Each needs
   machinery this does not have, and every one of them has a plausible wrong
   answer available -- which is exactly why none may fall through. */
static bool	zf_known(char c)
{
	return (ft_strchr("fszjFkv@oOnupULCq%P-", c) != NULL);
}

/* Reject the whole expansion on the first flag we do not implement. */
bool	zf_check(t_shell *state, t_zflags *f, t_token *tt)
{
	int	i;

	i = 0;
	while (i < f->n)
	{
		if (!zf_known(f->set[i]))
			return (zf_bad(state, tt, f->set[i]), false);
		i++;
	}
	return (true);
}

/* (p): the (s:...:) and (j:...:) arguments carry print-style escapes, so
   `${(ps:\n:)x}` splits on a NEWLINE and not on a backslash followed by an
   n.  Done in place -- unescaping only ever shortens -- once, before the
   arguments are used, so no consumer has to know the flag existed.
     The escape map is ansic_simple(), the one $'...' already uses, so \n
   means the same byte in both spellings.  An escape it does not know keeps
   its backslash, exactly as $'...' leaves an unknown escape alone. */
static void	zf_unesc_one(char *s)
{
	int	r;
	int	w;
	int	v;

	if (!s)
		return ;
	r = 0;
	w = 0;
	while (s[r])
	{
		v = -1;
		if (s[r] == '\\' && s[r + 1])
			v = ansic_simple(s[r + 1]);
		if (v >= 0)
		{
			s[w++] = (char)v;
			r++;
		}
		else
			s[w++] = s[r];
		r++;
	}
	s[w] = '\0';
}

void	zf_unesc(t_zflags *f)
{
	if (!zf_has(f, 'p'))
		return ;
	zf_unesc_one(f->sep);
	zf_unesc_one(f->join);
}

/* Join the list.  A NULL separator means the flag took no argument, which
   for (j) is zsh's "join with nothing" and not "join with a space". */
char	*zl_join(t_vec *l, const char *sep)
{
	t_string	out;
	size_t		i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (i < l->len)
	{
		if (i++ > 0 && sep)
			vec_push_str(&out, (char *)sep);
		vec_push_str(&out, ((char **)l->ctx)[i - 1]);
	}
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}
