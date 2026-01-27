/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_subshell.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 21:16:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/19 21:16:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

char	*parse_quote(t_deque_tok *tokens, char **str, char q);

/* handle subshell $(...) with nested parentheses and quoted segments */
char	*tokenize_subshell(t_deque_tok *tokens, char **str)
{
	int		depth;
	char	*res;

	(*str) += 2;
	depth = 1;
	while (**str && depth > 0)
	{
		if (**str == '\\')
			advance_bs(str);
		else if (**str == '\'' || **str == '"')
		{
			res = parse_quote(tokens, str, **str);
			if (res)
				return (res);
		}
		else
		{
			if (**str == '(')
				depth++;
			else if (**str == ')')
				depth--;
			(*str)++;
		}
	}
	return (0);
}
