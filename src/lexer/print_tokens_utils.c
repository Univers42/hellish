/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print_tokens_utils.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 20:32:52 by marvin            #+#    #+#             */
/*   Updated: 2026/01/19 20:32:52 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "sys.h"

/* Return the display name for a token. For TT_WORD tokens that are fully
   surrounded by matching quote characters we show TOKEN_DQ / TOKEN_SQ
   instead of "TT_WORD" -- it makes it much easier to spot quoting issues
   in the debug table at a glance. Everything else goes through tt_to_str. */
const char	*get_token_display_name(t_token *curr)
{
	unsigned char	fst;
	unsigned char	lst;

	if (curr->tt == TT_WORD && curr->len >= 2)
	{
		fst = ((unsigned char *)curr->start)[0];
		lst = ((unsigned char *)curr->start)[curr->len - 1];
		if (fst == '"' && lst == '"')
			return (TOKEN_DQ);
		else if (fst == '\'' && lst == '\'')
			return (TOKEN_SQ);
	}
	return (tt_to_str(curr->tt));
}
