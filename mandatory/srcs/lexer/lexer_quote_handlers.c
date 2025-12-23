/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer_quote_handlers.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 15:08:21 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/22 20:42:13 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "inlexer.h"

int	handle_quotes(t_lexer_st *state)
{
	char	quote;

	quote = state->input[*state->i];
	(*state->i)++;
	while (state->input[*state->i] && state->input[*state->i] != quote)
	{
		d_str_append_char(state->curr_tok_str, state->input[*state->i]);
		(*state->i)++;
	}
	if (state->input[*state->i] == quote)
		(*state->i)++;
	else
		return (0);
	add_token_to_vec(state->vec, state->curr_tok_str, TOK_WORD);
	return (1);
}
