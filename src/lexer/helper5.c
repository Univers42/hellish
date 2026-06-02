/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper5.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:05:17 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:05:17 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

/* unify single/double-quote advance and prompt handling */
char	*parse_quote(t_deque_tok *tokens, char **str, char q)
{
	if (q == '\'')
	{
		if (advance_squoted(str))
			return (tokens->looking_for = '\'', LEXER_SQUOTE_PROMPT);
	}
	else if (q == '"')
	{
		if (advance_dquoted(str))
			return (tokens->looking_for = '"', LEXER_DQUOTE_PROMPT);
	}
	return (0);
}
