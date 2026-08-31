/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_xform.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Drop duplicates, keeping the FIRST of each run.  zsh's (u) does not sort,
   so `${(u)x}` on `c a c b` is `c a b` and not `a b c`; combining it with
   (o) is how you ask for both, and the two staying separate is what lets
   `${(u)path}` de-duplicate $PATH without reordering it -- which would
   change which binary wins. */
void	zl_uniq(t_zflags *f, t_vec *l)
{
	char	**a;
	size_t	i;
	size_t	j;

	if (!zf_has(f, 'u'))
		return ;
	a = (char **)l->ctx;
	i = 0;
	while (i < l->len)
	{
		j = i + 1;
		while (j < l->len)
		{
			if (!ft_strcmp(a[i], a[j]))
				zl_erase(l, j);
			else
				j++;
		}
		i++;
	}
}

void	zl_swap(char **a, char **b)
{
	char	*t;

	t = *a;
	*a = *b;
	*b = t;
}

/* Remove one element, freeing it and closing the gap. */
void	zl_erase(t_vec *l, size_t at)
{
	char	**a;

	a = (char **)l->ctx;
	xfree(a[at]);
	while (at + 1 < l->len)
	{
		a[at] = a[at + 1];
		at++;
	}
	l->len--;
}

/* (U) (L) (C): upper, lower, capitalise-first.  Byte-wise, which is what
   zsh does outside a multibyte locale and what every use in the corpus
   wants -- these appear on branch names and option words, not on prose. */
char	*zf_case(const char *v, char how)
{
	char	*out;
	int		i;

	out = ft_strdup(v);
	if (!out)
		return (out);
	i = -1;
	while (out[++i])
	{
		if (how == 'L' || (how == 'C' && i > 0))
			out[i] = (char)ft_tolower((unsigned char)out[i]);
		else
			out[i] = (char)ft_toupper((unsigned char)out[i]);
	}
	return (out);
}

/* Run every element through the per-element flags, in zsh's own order:
   the value is resolved (P) before it is reshaped (U/L/C), reshaped before
   it is rendered (%), and quoted (q) last so the quoting describes what
   will actually be emitted. */
void	zl_map(t_shell *state, t_zflags *f, t_vec *l)
{
	size_t	i;

	if (!zf_has(f, 'P') && !zf_has(f, 'U') && !zf_has(f, 'L')
		&& !zf_has(f, 'C') && !zf_has(f, '%') && !zf_has(f, 'q'))
		return ;
	i = 0;
	while (i < l->len)
	{
		((char **)l->ctx)[i] = zx_one(state, f, ((char **)l->ctx)[i]);
		i++;
	}
}
