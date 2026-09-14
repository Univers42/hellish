/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_arith_elem.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "arith.h"

/* Array elements as the arithmetic evaluator names them: `a[i]`, `a[i+1]`,
   `a[a[0]]`, `m[key]` -- no braces, no dollar, the way bash reads
   $(( a[i] * 2 )) and (( sum += a[i] )).

   The lexer now hands such a name over whole, subscript included (see
   lex_subscript); this is where it means something. bash printed 3 for
   `a=(1 2 3); echo $(( a[2] ))` and hellish printed nothing at all -- the
   `[` reached an operator lexer that had no reading of it, an error on
   the plain path, and on the cached path a token that never advanced, so
   the token vector grew until the allocator gave up. `$(( a[a[0]] ))`
   hung. None of it was reachable through ${a[i]}, which is why 4894 golden
   cases had not noticed. */

/* name[sub] -> name length and subscript length; 0 when not that shape.
   The subscript ends at ITS bracket, which counting handles: a[a[0]]. */
static int	split_elem(const char *s, int len, int *sublen)
{
	int	nl;
	int	close;

	nl = 0;
	while (nl < len && s[nl] != '[')
		nl++;
	if (nl == 0 || nl >= len)
		return (0);
	close = subscript_close(s, len, nl);
	if (close != len - 1)
		return (0);
	*sublen = close - nl - 1;
	return (nl);
}

/* The subscript is arithmetic itself. */
static long	sub_number(t_shell *state, const char *sub, int sublen)
{
	char	*r;
	long	idx;

	r = arith_expand(state, sub, sublen);
	idx = 0;
	if (r)
		idx = ft_atol(r);
	xfree(r);
	return (idx);
}

/* The element's text, owned; NULL when unset, or when `name` is not of the
   name[sub] shape at all. An associative array takes the subscript as its
   key, as bash does; an indexed one evaluates it, negative counting from
   the end; a scalar answers for index 0 only. */
char	*arith_elem_get(t_shell *state, const char *name, int len)
{
	char	*val;
	int		nl;
	int		sublen;
	long	idx;

	nl = split_elem(name, len, &sublen);
	if (!nl)
		return (NULL);
	val = env_expand_n(state, (char *)name, nl);
	if (assoc_is(val))
		return (assoc_get(val, name + nl + 1, sublen));
	idx = sub_number(state, name + nl + 1, sublen);
	if (arr_is(val))
		return (arr_get_idx(val, sub_to_index(state, idx, arr_count(val))));
	return (zn_scalar_pick(state, val, idx));
}

/* (( a[i] = v )), (( a[i]++ )), (( m[k] += 1 )): the store goes to the
   element. Before this the evaluator set a variable literally named
   "a[i]", which nothing else in the shell could ever read back. */
void	arith_elem_set(t_shell *state, const char *name, int len,
			const char *nv)
{
	char	*val;
	char	*fresh;
	int		nl;
	int		sublen;
	long	idx;

	nl = split_elem(name, len, &sublen);
	if (!nl)
		return ;
	val = env_expand_n(state, (char *)name, nl);
	if (assoc_is(val))
		fresh = assoc_with_set(val, name + nl + 1, sublen, nv);
	else
	{
		idx = sub_number(state, name + nl + 1, sublen);
		if (arr_is(val))
			idx = sub_to_index(state, idx, arr_count(val));
		fresh = arr_with_set(val, idx, nv);
	}
	env_set(&state->env, env_create(ft_strndup(name, nl), fresh, true));
}

/* The three names the evaluator resolves through the expander rather than
   the environment, as owned text or NULL: zsh's `+name[key]`, zsh's
   `$#name` (arriving as "#name"), and a subscripted element. */
char	*arith_var_special(t_shell *state, const char *name, int len)
{
	if (len > 1 && name[0] == '+')
		return (zsh_param(state, name, len));
	if (len > 1 && name[0] == '#')
		return (expand_strlen(state, name + 1, len - 1));
	if (ft_memchr(name, '[', len))
		return (arith_elem_get(state, name, len));
	return (NULL);
}
