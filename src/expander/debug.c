/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:23:39 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:23:39 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	print_redir_err(t_shell *state,
		t_token_old full_token, char *expanded_name)
{
	if (!expanded_name)
	{
		if (full_token.present && full_token.start)
			ft_eprintf("%s: %.*s: ambiguous redirect\n",
				state->context, full_token.len, full_token.start);
		else
			ft_eprintf("%s: ambiguous redirect\n", state->context);
	}
	else
		ft_eprintf("%s: %s: %s\n", state->context,
			expanded_name, strerror(errno));
}
