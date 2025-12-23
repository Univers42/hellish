/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer_handlers.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 15:07:49 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/22 20:42:09 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "inlexer.h"

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
