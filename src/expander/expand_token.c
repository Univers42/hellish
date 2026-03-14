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

char	*expand_param_format(t_shell *state, const char *s, int slen);

void	expand_token(t_shell *state, t_token	*curr_tt)
{
	char	*temp;
	char	*fmt;

	if (curr_tt->len == 0)
	{
		if (curr_tt->start && (*curr_tt->start == '\''
				|| *curr_tt->start == '"'))
		{
			curr_tt->start = "";
			curr_tt->len = 0;
			curr_tt->allocated = false;
			return ;
		}
		curr_tt->start = "$";
		curr_tt->len = 1;
		curr_tt->allocated = false;
		return ;
	}
	fmt = expand_param_format(state, curr_tt->start, curr_tt->len);
	if (fmt)
	{
		curr_tt->start = fmt;
		curr_tt->len = ft_strlen(fmt);
		curr_tt->allocated = true;
		return ;
	}
	temp = env_expand_n(state, curr_tt->start, curr_tt->len);
	curr_tt->start = temp;
	if (curr_tt->start)
		curr_tt->len = ft_strlen(temp);
	else
		curr_tt->len = 0;
	curr_tt->allocated = false;
}
