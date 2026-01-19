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

// Helper to get the display name for a token
const char	*get_token_display_name(t_token *curr)
{
	unsigned char	fst;
	unsigned char	lst;

	if (curr->tt == TT_WORD && curr->len >= 2)
	{
		fst = ((unsigned char *)curr->start)[0];
		lst = ((unsigned char *)curr->start)[curr->len - 1];
		if (fst == '"' && lst == '"')
			return ("TT_DQWORD");
		else if (fst == '\'' && lst == '\'')
			return ("TT_SQWORD");
	}
	return (tt_to_str(curr->tt));
}
