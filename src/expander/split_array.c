/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   split_array.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* Field emission for deferred array expansions — the exact analogues of
   emit_positional_at / emit_positional_split with the element list as
   the source. Field breaks go BETWEEN elements only (the first element
   joins any accumulated prefix text, the last joins any suffix — the
   pre"${a[@]}"post rule, same as "$@"). A scalar variable counts as one
   element; an unset one contributes nothing. */

/* Quoted "${arr[@]}": one verbatim field per element. */
void	emit_array_at(t_shell *state, const char *name, t_ast_node *curr_node,
			t_vec_nd *ret)
{
	char		*val;
	const char	*cur;
	const char	*v;
	long		idx;
	int			nth[2];

	val = env_expand(state, (char *)name);
	if (!val)
		return ;
	if (assoc_is(val))
		return (emit_assoc_fields(val, curr_node, ret, 0));
	if (!arr_is(val))
		return (push_new_env_child(curr_node, ft_strdup(val)));
	cur = val + 1;
	nth[0] = 0;
	while (arr_next(&cur, &idx, &v, &nth[1]))
	{
		if (nth[0]++ > 0)
			push_and_reinit_curr_node(ret, curr_node);
		push_new_env_child(curr_node, ft_strndup(v, nth[1]));
	}
}

/* One field per associative element: values (want_keys 0) or keys
   (want_keys 1). Shared by "${h[@]}" and "${!h[@]}" emission. */
void	emit_assoc_fields(char *val, t_ast_node *curr_node, t_vec_nd *ret,
			int want_keys)
{
	const char	*cur;
	const char	*k;
	const char	*v;
	int			kl;
	int			nth[2];

	cur = val + 1;
	nth[0] = 0;
	while (assoc_next(&cur, &k, &kl, &v, &nth[1]))
	{
		if (nth[0]++ > 0)
			push_and_reinit_curr_node(ret, curr_node);
		if (want_keys)
			push_new_env_child(curr_node, ft_strndup(k, kl));
		else
			push_new_env_child(curr_node, ft_strndup(v, nth[1]));
	}
}

/* Unquoted ${arr[@]} / ${arr[*]}: one field per element, each element
   itself IFS-split, exactly like unquoted $@. */
void	emit_array_split(t_shell *state, const char *name,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	char		*val;
	char		*elem;
	const char	*cur;
	const char	*v;
	long		idx;
	int			nth[2];

	val = env_expand(state, (char *)name);
	if (!val)
		return ;
	if (assoc_is(val))
		return (emit_assoc_fields(val, curr_node, ret, 0));
	if (!arr_is(val))
		return (split_value(state, val, curr_node, ret));
	cur = val + 1;
	nth[0] = 0;
	while (arr_next(&cur, &idx, &v, &nth[1]))
	{
		if (nth[0]++ > 0)
			push_and_reinit_curr_node(ret, curr_node);
		elem = ft_strndup(v, nth[1]);
		split_value(state, elem, curr_node, ret);
		xfree(elem);
	}
}

/* Emit one field per index (indexed) or key (assoc) for a deferred
   ${!name[@]} keys-marker; `name` still has its leading '!'. */
void	emit_keys_fields(t_shell *state, const char *name,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	char		*val;
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;
	int			nth;

	val = env_expand(state, (char *)name + 1);
	if (!val)
		return ;
	if (assoc_is(val))
		return (emit_assoc_fields(val, curr_node, ret, 1));
	cur = "";
	if (arr_is(val))
		cur = val + 1;
	nth = 0;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		if (nth++ > 0)
			push_and_reinit_curr_node(ret, curr_node);
		push_new_env_child(curr_node, ft_itoa((int)idx));
	}
}
