/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   split_array2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* One verbatim field per element of an already-resolved value.  This is the
   whole of what "${arr[@]}" does once the name has been looked up, factored
   out so the zsh flag path can reach it: `${(f)$(cmd)}` has a list but no
   variable to find it under, so it parks the ENCODED VALUE in the deferral
   registry and lands here directly.

   A value that is not an array is one field, and an unset one is none --
   the same two edge cases "${x[@]}" has always had, now shared rather than
   restated. */
void	emit_val_at(const char *val, t_ast_node *curr_node, t_vec_nd *ret)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			nth[2];

	if (!val)
		return ;
	if (assoc_is(val))
		return (emit_assoc_fields((char *)val, curr_node, ret, 0));
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
