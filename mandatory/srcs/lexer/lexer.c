/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 14:50:46 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/22 20:50:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "inlexer.h"

int	add_token_to_vec(t_tok_vec *vector, t_d_str *d_str, t_tok_type type)
{
	t_toke	*token;

	token = malloc(sizeof(t_toke));
	if (!token)
		return (0);
	if (d_str_init(&token->value, d_str->len + 1) == -1)
	{
		free(token);
		return (0);
	}
	d_str_append_str(&token->value, d_str->str);
	token->type = type;
	if (vector_append(vector, token) == -1)
	{
		d_str_free(&token->value);
		free(token);
		return (0);
	}
	d_str_reset(d_str);
	return (1);
}

static int	lexer_process_char(t_lexer_st *state)
{
	char	current_char;

	current_char = state->input[*state->i];
	if (is_special_char(current_char))
		finalize_word(state);
	if (is_whitespace(current_char))
		(*state->i)++;
	else if (is_pipe(current_char))
		handle_pipe(state);
	else if (is_redirect(current_char))
		handle_redirect(state);
	else if (is_quote(current_char))
	{
		if (!handle_quotes(state))
			return (0);
	}
	else
	{
		d_str_append_char(state->curr_tok_str, current_char);
		(*state->i)++;
	}
	return (1);
}

t_tok_vec	*lexer(const char *input)
{
	t_lexer_st	state;
	t_tok_vec	*vector;
	t_d_str		curr_token_str;
	size_t		i;

	vector = vector_init(16);
	if (!vector || d_str_init(&curr_token_str, 16) == -1)
		return (NULL);
	i = 0;
	state = (t_lexer_st){input, vector, &curr_token_str, &i};
	while (state.input[i])
	{
		if (!lexer_process_char(&state))
		{
			vector_free(vector);
			d_str_free(&curr_token_str);
			return (NULL);
		}
	}
	finalize_word(&state);
	d_str_free(&curr_token_str);
	return (vector);
}
