/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_emit2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:35:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:35:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Emit a value that may be an encoded array: unpack it into the list the
   emitter already knows how to place, so a filtered array becomes fields
   and a filtered scalar becomes one word. Consumes `val`. */
void	zf_emit_value(t_shell *state, t_zflags *f, t_token *tt, char *val)
{
	t_vec	l;

	l = zl_from(state, f, val);
	xfree(val);
	zf_emit(state, f, tt, &l);
}
