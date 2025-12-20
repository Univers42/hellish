#include "lexer.h"

int	is_whitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n');
}

static void	process_word_token(const char *input, size_t *i, t_dynamic_str *d_str)
{
	while (input[*i] && !is_whitespace(input[*i]))
	{
		d_str_append_char(d_str, input[*i]);
		(*i)++;
	}
}

static int	add_token_to_vector(t_token_vector *vector, t_dynamic_str *d_str, t_token_type type)
{
	t_token	*token;

	token = malloc(sizeof(t_token));
	if (d_str->len == 0)
		return (0);
	token = malloc(sizeof(t_token));
	if (!token)
		return (0);
	d_str_init(&token->value, d_str->len);
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

t_token_vector	*lexer(const char *input)
{
	t_token_vector	*vector;
	t_dynamic_str	current_token_str;
	size_t			i;

	vector = vector_init(10);
	if (!vector)
		return (NULL);
	if (d_str_init(&current_token_str, 10) == -1)
	{
		vector_free(vector);
		return (NULL);
	}
	i = 0;
	while (input[i])
	{
		if (is_whitespace(input[i]))
		{
			i++;
			continue ;
		}
		process_word_token(input, &i, &current_token_str);
		add_token_to_vector(vector, &current_token_str, TOKEN_WORD);
	}
	return (vector);
}
