/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug_tokens.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:08 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../shell.h"
#include <stdio.h>

char	*tt_to_str_p2(t_tt tt)
{
	if (tt == TT_END)
		return ("TT_END");
	if (tt == TT_WORD)
		return ("TT_WORD");
	if (tt == TT_REDIRECT_LEFT)
		return ("TT_REDIRECT_LEFT");
	if (tt == TT_REDIRECT_RIGHT)
		return ("TT_REDIRECT_RIGHT");
	if (tt == TT_APPEND)
		return ("TT_APPEND");
	if (tt == TT_PIPE)
		return ("TT_PIPE");
	if (tt == TT_BRACE_LEFT)
		return ("TT_BRACE_LEFT");
	if (tt == TT_BRACE_RIGHT)
		return ("TT_BRACE_RIGHT");
	if (tt == TT_OR)
		return ("TT_OR");
	if (tt == TT_AND)
		return ("TT_AND");
	ft_assert(false);
	return (0);
}

char	*tt_to_str(t_tt tt)
{
	if (tt == TT_SEMICOLON)
		return ("TT_SEMICOLON");
	if (tt == TT_HEREDOC)
		return ("TT_HEREDOC");
	if (tt == TT_NEWLINE)
		return ("TT_NEWLINE");
	if (tt == TT_SQWORD)
		return ("TT_QWORD");
	if (tt == TT_DQWORD)
		return ("TT_DQWORD");
	if (tt == TT_ENVVAR)
		return ("TT_ENVVAR");
	if (tt == TT_DQENVVAR)
		return ("TT_DQENVVAR");
	return (tt_to_str_p2(tt));
}

/* changed: accept the token-wrapper and read from its internal deque */
void	print_tokens(t_deque_tt tokens)
{
	t_token	curr;
	int		i;

	i = 0;
	ft_printf("------- PRINTING TOKENS --------\n");
	while ((size_t)i < tokens.deqtok.len)
	{
		curr = *(t_token *)ft_deque_idx(&tokens.deqtok, i);
		ft_printf("%s: >%.*s<\n", tt_to_str(curr.tt), curr.len, curr.start);
		i++;
	}
	ft_printf("------- DONE --------\n");
}
