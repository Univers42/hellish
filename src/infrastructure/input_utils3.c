/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:31:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:53:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"
#include "helpers.h"

char	*expand_history(t_shell *state, const char *input);

bool	ends_with_bs_nl(t_string s)
{
	size_t	i;
	bool	unterminated;

	if (s.len == 0)
		return (false);
	i = s.len;
	unterminated = false;
	if (((char *)s.ctx)[--i] != '\n')
		return (false);
	while (i > 0)
	{
		i--;
		if (((char *)s.ctx)[i] == '\\')
			unterminated = !unterminated;
		else
			break ;
	}
	return (unterminated);
}

void	extend_bs(t_shell *state)
{
	char	*prompt;

	while (ends_with_bs_nl(state->input))
	{
		vec_pop(&state->input);
		vec_pop(&state->input);
		prompt = ft_strdup("> ");
		if (readline_cmd(state, &prompt))
			return ;
	}
}

bool	is_empty_token_list(t_deque_tok *tokens)
{
	if (tokens->deqtok.len < 2)
		return (true);
	if (tokens->deqtok.len == 2
		&& ((t_token *)deque_idx(&tokens->deqtok, 0))->tt == TT_NEWLINE)
		return (true);
	return (false);
}
