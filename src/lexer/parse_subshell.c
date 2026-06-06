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

/* Track nesting depth as we scan for the matching `)`. Opening `(` bumps
   depth; closing `)` decrements. We advance *str after updating so the
   scanner never re-visits the same character. */
static void	update_depth(char **str, int *depth)
{
	if (**str == '(')
		(*depth)++;
	else if (**str == ')')
		(*depth)--;
	(*str)++;
}

/* Scan over a $(...) command substitution at lex time without tokenising
   its interior. We honour nesting and quoted spans so a `"` or `)` inside
   a quoted string does not terminate the outer subshell. The span is later
   re-lexed by the expander. Returns NULL on success or a continuation prompt
   if the `)` was never found before end-of-input. */
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
			update_depth(str, &depth);
	}
	if (depth > 0)
		return (tokens->looking_for = ')', "subshell> ");
	return (0);
}
