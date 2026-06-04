/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_cmd_simple_word.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:29:56 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:29:56 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

int	expand_simple_cmd_word(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	int	o;

	if (!exp->found_first && is_export(*exp->curr))
		exp->export = true;
	o = word_slab_push(1);
	expand_word_ro(state, exp->curr, &ret->argv, false);
	word_slab_push(o);
	if (get_g_sig()->should_unwind)
		return (1);
	exp->found_first = true;
	return (0);
}
