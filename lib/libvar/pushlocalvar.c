/* ************************************************************************** */
/* */
/* :::      ::::::::   */
/* pushlocalvars.c                                    :+:      :+:    :+:   */
/* +:+ +:+         +:+     */
/* By: <tu_login>                             <+       +#+        +:+     */
/* Kk;k;                                           +#+#+#+#+#+   +#+           */
/* Created: 2024/01/01 00:00:00 by <tu_login>          #+#    #+#             */
/* Updated: 2024/01/01 00:00:00 by <tu_login>         ###   ########.fr       */
/* */
/* ************************************************************************** */

#include "libvar.h"

t_localvar_list	*pushlocalvars(int push)
{
	t_localvar_list	*ll;
	t_localvar_list	*top;
	t_var_state		*state;

	state = get_var_state();
	top = state->localvar_stack;
	if (push)
	{
		INTOFF;
		ll = ckmalloc(sizeof(*ll));
		ll->lv = NULL;
		ll->next = top;
		state->localvar_stack = ll;
		INTON;
	}
	return (top);
}

