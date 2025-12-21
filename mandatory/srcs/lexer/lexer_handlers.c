/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer_handlers.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 15:07:49 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/21 16:50:28 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

void	finalize_word(t_lexer_st *state)
{
	if (state->curr_tok_str->len > 0)
		add_token_to_vec(state->vec, state->curr_tok_str, TOK_WORD);
}

void	handle_pipe(t_lexer_st *state)
{
	finalize_word(state);
	d_str_append_char(state->curr_tok_str, state->input[*state->i]);
	(*state->i)++;
	add_token_to_vec(state->vec, state->curr_tok_str, TOK_PIPE);
}

void	handle_whitespace(t_lexer_st *state)
{
	finalize_word(state);
	while (state->input[*state->i] && is_whitespace(state->input[*state->i]))
		(*state->i)++;
}
