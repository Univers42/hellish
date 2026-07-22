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

/* Split one expanded element into "[sub]=value": true with sub/subl/val
   filled when it starts '[' and has a "]=", false (bare value) otherwise.
   Quoting that would suppress the subscript is already lost by word
   expansion — a documented element-level v1 divergence. */
int	parse_sub_elem(char *elem, char **sub, int *subl, char **val)
{
	char	*rb;

	if (elem[0] != '[')
		return (0);
	rb = ft_strchr(elem, ']');
	if (!rb || rb[1] != '=')
		return (0);
	*sub = elem + 1;
	*subl = (int)(rb - (elem + 1));
	*val = rb + 2;
	return (1);
}

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

/* Build an associative value from [key]=value elements. A plain `=`
   assignment starts from an empty array (bash replaces the whole map);
   `+=` starts from the current value and merges. Bare elements (no
   [key]=) are ignored, as bash rejects them for assoc arrays. */
static char	*build_assoc(t_vec *args, const char *base, int append)
{
	char	*cur;
	char	*nv;
	char	*sub;
	char	*val;
	int		subl;
	size_t	i;

	if (append && assoc_is(base))
		cur = ft_strdup(base);
	else
		cur = ft_strdup((char [2]){ARR_ASSOC_MAGIC});
	i = 0;
	while (i < args->len)
	{
		if (parse_sub_elem(((char **)args->ctx)[i++], &sub, &subl, &val))
		{
			nv = assoc_with_set(cur, sub, subl, val);
			xfree(cur);
			cur = nv;
		}
	}
	return (cur);
}

/* Decide the encoded form for a compound array assignment and build it:
   associative when the target is assoc-typed, indexed-with-subscripts
   when any element is [i]=v, else the plain sequential fast path. base
   aliases the current value (never freed here). */
char	*build_array_value(t_shell *state, t_executable_cmd *ret,
			t_env *ev, t_vec *args, int append)
{
	const char	*base;

	base = env_expand(state, ev->key);
	if (is_assoc_target(ret, base))
		return (build_assoc(args, base, append));
	if (has_subscript(args))
		return (build_indexed_sub(state, args, base, append));
	if (append)
		return (arr_from_elems((char **)args->ctx, (int)args->len, base));
	return (arr_from_elems((char **)args->ctx, (int)args->len, NULL));
}
