/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array_assign2.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* Compound-init with explicit subscripts: arr=([k]=v ...). Bash accepts
   `[subscript]=value` elements in BOTH indexed (subscript is arithmetic)
   and associative (subscript is a literal key) arrays; plain `value`
   elements fill the next sequential index. This file classifies the
   target and builds the associative form; the indexed form lives in
   expand_array_assign3.c. */

/* Does any element carry an explicit [subscript]= prefix? Plain lists
   (a=(x y z)) skip the incremental builder and keep the fast path. */
int	has_subscript(t_vec *args)
{
	size_t	i;
	char	*sub;
	char	*val;
	int		subl;

	i = 0;
	while (i < args->len)
	{
		if (parse_sub_elem(((char **)args->ctx)[i], &sub, &subl, &val))
			return (1);
		i++;
	}
	return (0);
}

/* Is the target associative? Either it already holds an assoc value
   (declare -A h; h=(...)) or the same command carries a declare/typeset
   -A flag (declare -A h=(...)). base is the target's current value. */
static int	is_assoc_target(t_executable_cmd *ret, const char *base)
{
	size_t	i;
	char	*a;

	if (assoc_is(base))
		return (1);
	if (!is_assign_builtin(ret))
		return (0);
	i = 1;
	while (i < ret->argv.len)
	{
		a = ((char **)ret->argv.ctx)[i++];
		if (a && a[0] == '-' && ft_strchr(a, 'A'))
			return (1);
	}
	return (0);
}

/* assoc_with_set on `cur`, freeing the old buffer; returns the new one
   (the assoc twin of idx_set_free in expand_array_assign3.c). */
static char	*assoc_set_free(char *cur, const char *sub, int subl,
				const char *val)
{
	char	*nv;

	nv = assoc_with_set(cur, sub, subl, val);
	xfree(cur);
	return (nv);
}

/* Build an associative value from [key]=value elements. A plain `=`
   assignment starts from an empty array (bash replaces the whole map);
   `+=` starts from the current value and merges. Bare elements (no
   [key]=) are ignored, as bash rejects them for assoc arrays.
     An empty key -- `M=([$unset]=v)` -- is refused here for the same reason
   assoc_elem_assign refuses it: a map has no element 0 to fall back on, so
   accepting it stores under a key the script never named.  The map built so
   far is returned intact, because bad_subscript does not always exit: inside
   a sourced file it abandons only the file. */
static char	*build_assoc(t_shell *state, t_arr_assign *aa, const char *base)
{
	char	*cur;
	char	*sub;
	char	*val;
	int		subl;
	size_t	i;

	if (aa->append && assoc_is(base))
		cur = ft_strdup(base);
	else
		cur = ft_strdup((char [2]){ARR_ASSOC_MAGIC});
	i = 0;
	while (i < aa->args->len)
	{
		if (!parse_sub_elem(((char **)aa->args->ctx)[i++], &sub, &subl, &val))
			continue ;
		if (subl <= 0)
			return (bad_subscript(state, aa->ev->key, ""), cur);
		cur = assoc_set_free(cur, sub, subl, val);
	}
	return (cur);
}

/* Decide the encoded form for a compound array assignment and build it:
   associative when the target is assoc-typed, indexed-with-subscripts
   when any element is [i]=v, else the plain sequential fast path. base
   aliases the current value (never freed here). aa bundles the half-built
   env entry, the element list and the += flag (norm: four args max). */
char	*build_array_value(t_shell *state, t_executable_cmd *ret,
			t_arr_assign *aa)
{
	const char	*base;

	base = env_expand(state, aa->ev->key);
	if (is_assoc_target(ret, base))
		return (build_assoc(state, aa, base));
	if (has_subscript(aa->args))
		return (build_indexed_sub(state, aa->args, base, aa->append));
	if (aa->append)
		return (arr_from_elems((char **)aa->args->ctx,
				(int)aa->args->len, base));
	return (arr_from_elems((char **)aa->args->ctx,
			(int)aa->args->len, NULL));
}
