#include "lexer.h"

int		add_token_to_vector(t_token_vector *vector, t_dynamic_str *d_str, t_token_type type)
{
	t_token	*token;

	if (d_str->len == 0)
		return (1);

	token = malloc(sizeof(t_token));
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

static int lexer_process_char(t_lexer_state *state)
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
		d_str_append_char(state->current_token_str, current_char);
		(*state->i)++;
	}
	return (1);
}

t_token_vector	*lexer(const char *input)
{
	t_lexer_state	state;
	t_token_vector	*vector;
	t_dynamic_str	current_token_str;
	size_t			i;

	vector = vector_init(16);
	if (!vector || d_str_init(&current_token_str, 16) == -1)
		return (NULL);
	i = 0;
	state = (t_lexer_state){input, vector, &current_token_str, &i};
	while (state.input[i])
	{
		if (!lexer_process_char(&state))
		{
			vector_free(vector);
			d_str_free(&current_token_str);
			return (NULL);
		}
	}
	finalize_word(&state);
	d_str_free(&current_token_str);
	return (vector);
}
