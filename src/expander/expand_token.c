/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_token.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:30:31 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:30:31 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	expand_token(t_shell *state, t_token	*curr_tt)
{
	char *temp;

	/* Handle empty var name specially: if the next char is a quote (e.g. $'' or ""),
	   treat as empty string; otherwise a bare '$' should remain as literal '$'. */
	if (curr_tt->len == 0)
	{
		if (curr_tt->start && (*curr_tt->start == '\'' || *curr_tt->start == '"'))
		{
			/* quoted empty expansion -> empty string */
			curr_tt->start = "";
			curr_tt->len = 0;
			curr_tt->allocated = false;
			return;
		}
		/* bare $ -> leave as literal dollar */
		curr_tt->start = "$";
		curr_tt->len = 1;
		curr_tt->allocated = false;
		return;
	}

	temp = env_expand_n(state, curr_tt->start, curr_tt->len);
	curr_tt->start = temp;
	if (curr_tt->start)
		curr_tt->len = ft_strlen(temp);
	else
		curr_tt->len = 0;
	curr_tt->allocated = false;
}