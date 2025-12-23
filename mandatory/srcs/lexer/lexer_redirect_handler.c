/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer_redirect_handler.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 15:13:41 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/22 20:42:18 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "inlexer.h"

static void	handle_redir_out(t_lexer_st *state)
{
	d_str_append_char(state->curr_tok_str, state->input[*state->i]);
	(*state->i)++;
	if (state->input[*state->i] == '>')
	{
		d_str_append_char(state->curr_tok_str, state->input[*state->i]);
		(*state->i)++;
		add_token_to_vec(state->vec, state->curr_tok_str, TOK_REDIR_APPEND);
	}
	else
		add_token_to_vec(state->vec, state->curr_tok_str, TOK_REDIR_OUT);
}

static void	handle_redir_in(t_lexer_st *state)
{
	d_str_append_char(state->curr_tok_str, state->input[*state->i]);
	(*state->i)++;
	if (state->input[*state->i] == '<')
	{
		d_str_append_char(state->curr_tok_str, state->input[*state->i]);
		(*state->i)++;
		add_token_to_vec(state->vec, state->curr_tok_str, TOK_HEREDOC);
	}
	else
		add_token_to_vec(state->vec, state->curr_tok_str, TOK_REDIR_IN);
}

void	handle_redirect(t_lexer_st *state)
{
	if (state->input[*state->i] == '>')
		handle_redir_out(state);
	else
		handle_redir_in(state);
}
